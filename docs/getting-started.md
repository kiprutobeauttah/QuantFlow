# Getting Started with QuantFlow

## Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- Python 3.6+ (for data generation scripts)

### Build from Source

```bash
git clone https://github.com/yourusername/quantflow.git
cd quantflow
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## Your First Strategy

### 1. Create a Simple Moving Average Strategy

```cpp
#include "quantflow/strategy/strategy_base.hpp"
#include "quantflow/indicators/moving_average.hpp"

class MyFirstStrategy : public quantflow::strategy::Strategy {
public:
    MyFirstStrategy(const Symbol& symbol)
        : symbol_(symbol), sma_(20) {}
    
    void on_bar(const Bar& bar) override {
        sma_.update(bar.close);
        
        if (!sma_.is_ready()) return;
        
        // Buy when price crosses above SMA
        if (bar.close > sma_.value()) {
            context_->buy(symbol_, 100.0);
        }
    }

private:
    Symbol symbol_;
    SMA sma_;
};
```

### 2. Run a Backtest

```cpp
#include "quantflow/backtest/backtest_engine.hpp"

int main() {
    // Configure backtest
    BacktestConfig config;
    config.initial_cash = 100000.0;
    config.commission_rate = 0.001;
    
    // Create engine
    BacktestEngine engine(config);
    
    // Add strategy
    auto strategy = std::make_shared<MyFirstStrategy>("AAPL");
    engine.add_strategy(strategy);
    
    // Load data
    auto bars = data::CSVReader::read_bars("data/AAPL.csv");
    engine.add_data(bars);
    
    // Run
    engine.run();
    
    // Results
    auto results = engine.get_results();
    std::cout << "Return: " << results.total_return << "%" << std::endl;
    
    return 0;
}
```

### 3. Generate Sample Data

```bash
python3 scripts/generate_sample_data.py
```

This creates CSV files in `data/historical/` for testing.

## Next Steps

- Explore the [examples/](../examples/) directory
- Read the [API Reference](api-reference.md)
- Learn about [Strategy Development](strategies.md)
- Check out [Performance Optimization](performance.md)
