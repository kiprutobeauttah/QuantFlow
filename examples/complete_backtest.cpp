#include "quantflow/strategy/strategy_base.hpp"
#include "quantflow/indicators/moving_average.hpp"
#include "quantflow/indicators/momentum.hpp"
#include "quantflow/indicators/volatility.hpp"
#include "quantflow/backtest/backtest_engine.hpp"
#include "quantflow/backtest/performance_analyzer.hpp"
#include "quantflow/data/csv_reader.hpp"
#include <iostream>
#include <iomanip>

using namespace quantflow;
using namespace quantflow::strategy;
using namespace quantflow::indicators;
using namespace quantflow::backtest;

class MultiIndicatorStrategy : public Strategy {
public:
    MultiIndicatorStrategy(const Symbol& symbol)
        : symbol_(symbol),
          fast_sma_(10),
          slow_sma_(30),
          rsi_(14),
          bb_(20, 2.0),
          position_size_(100.0),
          in_position_(false) {}
    
    void on_init() override {
        std::cout << "Strategy initialized for " << symbol_ << std::endl;
    }
    
    void on_bar(const Bar& bar) override {
        if (bar.symbol != symbol_) return;
        
        fast_sma_.update(bar.close);
        slow_sma_.update(bar.close);
        rsi_.update(bar.close);
        bb_.update(bar.close);
        
        if (!fast_sma_.is_ready() || !slow_sma_.is_ready() || 
            !rsi_.is_ready() || !bb_.is_ready()) {
            return;
        }
        
        double fast_val = fast_sma_.value();
        double slow_val = slow_sma_.value();
        double rsi_val = rsi_.value();
        double bb_upper = bb_.upper_band();
        double bb_lower = bb_.lower_band();
        
        const Position* pos = context_->get_position(symbol_);
        bool has_position = pos && !pos->is_flat();
        
        // Entry signals
        if (!has_position) {
            // Bullish: Fast SMA crosses above slow, RSI not overbought
            if (fast_val > slow_val && rsi_val < 70 && bar.close < bb_upper) {
                context_->buy(symbol_, position_size_);
                in_position_ = true;
                entry_price_ = bar.close;
                std::cout << "BUY at " << bar.close 
                         << " | RSI: " << std::fixed << std::setprecision(2) << rsi_val
                         << " | Fast SMA: " << fast_val
                         << " | Slow SMA: " << slow_val << std::endl;
            }
        }
        // Exit signals
        else {
            bool exit_signal = false;
            std::string exit_reason;
            
            // Take profit at 5%
            if (bar.close >= entry_price_ * 1.05) {
                exit_signal = true;
                exit_reason = "Take Profit";
            }
            // Stop loss at 2%
            else if (bar.close <= entry_price_ * 0.98) {
                exit_signal = true;
                exit_reason = "Stop Loss";
            }
            // Technical exit: Fast SMA crosses below slow or RSI overbought
            else if (fast_val < slow_val || rsi_val > 75) {
                exit_signal = true;
                exit_reason = "Technical Exit";
            }
            
            if (exit_signal) {
                context_->sell(symbol_, position_size_);
                in_position_ = false;
                double pnl_pct = ((bar.close - entry_price_) / entry_price_) * 100.0;
                std::cout << "SELL at " << bar.close 
                         << " | Reason: " << exit_reason
                         << " | P&L: " << std::showpos << pnl_pct << "%" << std::noshowpos
                         << std::endl;
            }
        }
    }
    
    void on_fill(const Fill& fill) override {
        std::cout << "Fill: " << fill.quantity << " @ " << fill.price 
                 << " | Commission: $" << fill.commission << std::endl;
    }

private:
    Symbol symbol_;
    SMA fast_sma_;
    SMA slow_sma_;
    RSI rsi_;
    BollingerBands bb_;
    double position_size_;
    bool in_position_;
    double entry_price_;
};

void print_results(const BacktestResult& results) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "BACKTEST RESULTS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Final Equity:        $" << results.final_equity << std::endl;
    std::cout << "Total Return:        " << results.total_return << "%" << std::endl;
    std::cout << "Sharpe Ratio:        " << results.sharpe_ratio << std::endl;
    std::cout << "Max Drawdown:        " << results.max_drawdown << "%" << std::endl;
    
    std::cout << "\nTrade Statistics:" << std::endl;
    std::cout << "Total Trades:        " << results.total_trades << std::endl;
    std::cout << "Winning Trades:      " << results.winning_trades << std::endl;
    std::cout << "Losing Trades:       " << results.losing_trades << std::endl;
    std::cout << "Win Rate:            " << results.win_rate << "%" << std::endl;
    std::cout << "Profit Factor:       " << results.profit_factor << std::endl;
    
    std::cout << std::string(60, '=') << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "QuantFlow - Complete Backtest Example\n" << std::endl;
    
    // Configuration
    BacktestConfig config;
    config.initial_cash = 100000.0;
    config.commission_rate = 0.001;  // 0.1%
    config.slippage_bps = 5.0;       // 5 basis points
    
    // Create backtest engine
    BacktestEngine engine(config);
    
    // Add strategy
    auto strategy = std::make_shared<MultiIndicatorStrategy>("AAPL");
    engine.add_strategy(strategy);
    
    // Generate or load data
    std::vector<Bar> bars;
    
    if (argc > 1) {
        // Load from CSV file
        std::string filename = argv[1];
        std::cout << "Loading data from " << filename << "..." << std::endl;
        bars = data::CSVReader::read_bars(filename);
    } else {
        // Generate synthetic data
        std::cout << "Generating synthetic data..." << std::endl;
        double price = 150.0;
        for (int i = 0; i < 500; ++i) {
            Bar bar;
            bar.symbol = "AAPL";
            bar.timestamp = i * 86400000000000LL;  // Daily bars
            
            // Random walk with trend and mean reversion
            double trend = std::sin(i * 0.02) * 0.002;
            double noise = (rand() % 200 - 100) / 10000.0;
            price *= (1.0 + trend + noise);
            
            bar.open = price;
            bar.high = price * (1.0 + abs(rand() % 100) / 10000.0);
            bar.low = price * (1.0 - abs(rand() % 100) / 10000.0);
            bar.close = bar.low + (bar.high - bar.low) * (rand() % 100) / 100.0;
            bar.volume = 1000000 + rand() % 500000;
            bar.period = 86400000000000LL;
            
            bars.push_back(bar);
        }
    }
    
    std::cout << "Loaded " << bars.size() << " bars" << std::endl;
    std::cout << "\nRunning backtest...\n" << std::endl;
    
    // Add data and run
    engine.add_data(bars);
    engine.run();
    
    // Get and print results
    auto results = engine.get_results();
    print_results(results);
    
    // Portfolio summary
    const auto& portfolio = engine.get_portfolio();
    std::cout << "\nFinal Portfolio State:" << std::endl;
    std::cout << "Cash:                $" << portfolio.cash << std::endl;
    std::cout << "Equity:              $" << portfolio.equity << std::endl;
    std::cout << "Positions:           " << portfolio.num_positions() << std::endl;
    
    return 0;
}
