#include "quantflow/strategy/strategy_base.hpp"
#include "quantflow/indicators/moving_average.hpp"
#include "quantflow/indicators/volatility.hpp"
#include "quantflow/backtest/backtest_engine.hpp"
#include <iostream>

using namespace quantflow;
using namespace quantflow::strategy;
using namespace quantflow::indicators;
using namespace quantflow::backtest;

class MeanReversionStrategy : public Strategy {
public:
    MeanReversionStrategy(const Symbol& symbol)
        : symbol_(symbol),
          bb_(20, 2.0),
          position_size_(100.0) {}
    
    void on_bar(const Bar& bar) override {
        if (bar.symbol != symbol_) return;
        
        bb_.update(bar.close);
        
        if (!bb_.is_ready()) return;
        
        double upper = bb_.upper_band();
        double lower = bb_.lower_band();
        double middle = bb_.value();
        
        const Position* pos = context_->get_position(symbol_);
        bool has_position = pos && !pos->is_flat();
        
        if (bar.close < lower && !has_position) {
            context_->buy(symbol_, position_size_);
            std::cout << "BUY at " << bar.close << " (below lower band)" << std::endl;
        }
        else if (bar.close > middle && has_position) {
            context_->sell(symbol_, position_size_);
            std::cout << "SELL at " << bar.close << " (mean reversion)" << std::endl;
        }
    }

private:
    Symbol symbol_;
    BollingerBands bb_;
    double position_size_;
};

int main() {
    BacktestConfig config;
    config.initial_cash = 100000.0;
    
    BacktestEngine engine(config);
    auto strategy = std::make_shared<MeanReversionStrategy>("AAPL");
    engine.add_strategy(strategy);
    
    std::vector<Bar> bars;
    for (int i = 0; i < 200; ++i) {
        Bar bar;
        bar.symbol = "AAPL";
        bar.timestamp = i * 86400000000000LL;
        bar.close = 100.0 + std::sin(i * 0.2) * 15.0 + (rand() % 10 - 5);
        bars.push_back(bar);
    }
    
    engine.add_data(bars);
    engine.run();
    
    auto results = engine.get_results();
    std::cout << "\nBacktest Results:" << std::endl;
    std::cout << "Final Equity: $" << results.final_equity << std::endl;
    std::cout << "Total Return: " << results.total_return << "%" << std::endl;
    
    return 0;
}
