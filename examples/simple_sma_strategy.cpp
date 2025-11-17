#include "quantflow/strategy/strategy_base.hpp"
#include "quantflow/indicators/moving_average.hpp"
#include "quantflow/backtest/backtest_engine.hpp"
#include <iostream>

using namespace quantflow;
using namespace quantflow::strategy;
using namespace quantflow::indicators;
using namespace quantflow::backtest;

class SimpleSMAStrategy : public Strategy {
public:
    SimpleSMAStrategy(const Symbol& symbol, int fast_period, int slow_period)
        : symbol_(symbol),
          fast_sma_(fast_period),
          slow_sma_(slow_period),
          position_size_(100.0) {}
    
    void on_bar(const Bar& bar) override {
        if (bar.symbol != symbol_) return;
        
        fast_sma_.update(bar.close);
        slow_sma_.update(bar.close);
        
        if (!fast_sma_.is_ready() || !slow_sma_.is_ready()) return;
        
        double fast_val = fast_sma_.value();
        double slow_val = slow_sma_.value();
        
        const Position* pos = context_->get_position(symbol_);
        bool has_position = pos && !pos->is_flat();
        
        // Golden cross - buy signal
        if (fast_val > slow_val && !has_position) {
            context_->buy(symbol_, position_size_);
            std::cout << "BUY at " << bar.close << std::endl;
        }
        // Death cross - sell signal
        else if (fast_val < slow_val && has_position) {
            context_->sell(symbol_, position_size_);
            std::cout << "SELL at " << bar.close << std::endl;
        }
    }

private:
    Symbol symbol_;
    SMA fast_sma_;
    SMA slow_sma_;
    double position_size_;
};

int main() {
    BacktestConfig config;
    config.initial_cash = 100000.0;
    config.commission_rate = 0.001;
    
    BacktestEngine engine(config);
    
    auto strategy = std::make_shared<SimpleSMAStrategy>("AAPL", 10, 30);
    engine.add_strategy(strategy);
    
    // Generate sample data
    std::vector<Bar> bars;
    for (int i = 0; i < 100; ++i) {
        Bar bar;
        bar.symbol = "AAPL";
        bar.timestamp = i * 86400000000000LL;
        bar.close = 100.0 + std::sin(i * 0.1) * 10.0;
        bars.push_back(bar);
    }
    
    engine.add_data(bars);
    engine.run();
    
    auto results = engine.get_results();
    std::cout << "Final Equity: $" << results.final_equity << std::endl;
    std::cout << "Total Return: " << results.total_return << "%" << std::endl;
    
    return 0;
}
