# QuantFlow

High-performance C++ algorithmic trading system for backtesting and live trading.

## Features

- **Low-latency market data processing** - Lock-free queues, cache-aligned structures
- **Complete technical indicator library** - SMA, EMA, RSI, MACD, Bollinger Bands, ATR
- **Flexible strategy framework** - Event-driven architecture with easy customization
- **Portfolio and risk management** - Position tracking, P&L calculation, risk limits
- **Backtesting engine** - Historical simulation with realistic fills and slippage
- **Performance analytics** - Sharpe ratio, max drawdown, win rate, profit factor
- **Time series database** - In-memory and disk-based storage options

## Architecture

```
quantflow/
├── core/           # Type system, time utilities, constants
├── market_data/    # Live and historical data feeds
├── indicators/     # Technical analysis indicators
├── strategy/       # Strategy framework and base classes
├── execution/      # Order management and routing
├── portfolio/      # Position and portfolio tracking
├── risk/           # Risk management and validation
├── backtest/       # Backtesting engine and analytics
├── data/           # Time series database
└── utils/          # Lock-free queues, logging, threading
```

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Quick Start

### 1. Simple SMA Crossover Strategy

```cpp
#include "quantflow/strategy/strategy_base.hpp"
#include "quantflow/indicators/moving_average.hpp"
#include "quantflow/backtest/backtest_engine.hpp"

class SMAStrategy : public Strategy {
public:
    SMAStrategy(const Symbol& symbol, int fast, int slow)
        : symbol_(symbol), fast_sma_(fast), slow_sma_(slow) {}
    
    void on_bar(const Bar& bar) override {
        fast_sma_.update(bar.close);
        slow_sma_.update(bar.close);
        
        if (!fast_sma_.is_ready() || !slow_sma_.is_ready()) return;
        
        const Position* pos = context_->get_position(symbol_);
        bool has_position = pos && !pos->is_flat();
        
        if (fast_sma_.value() > slow_sma_.value() && !has_position) {
            context_->buy(symbol_, 100.0);
        } else if (fast_sma_.value() < slow_sma_.value() && has_position) {
            context_->sell(symbol_, 100.0);
        }
    }

private:
    Symbol symbol_;
    SMA fast_sma_, slow_sma_;
};
```

### 2. Run a Backtest

```cpp
BacktestConfig config;
config.initial_cash = 100000.0;
config.commission_rate = 0.001;

BacktestEngine engine(config);
auto strategy = std::make_shared<SMAStrategy>("AAPL", 10, 30);
engine.add_strategy(strategy);

// Load data
auto bars = data::CSVReader::read_bars("data/historical/AAPL.csv");
engine.add_data(bars);

// Run
engine.run();

// Results
auto results = engine.get_results();
std::cout << "Total Return: " << results.total_return << "%" << std::endl;
std::cout << "Sharpe Ratio: " << results.sharpe_ratio << std::endl;
```

## Examples

Run the included examples:

```bash
# Simple SMA crossover
./build/examples/simple_sma_strategy

# Mean reversion with Bollinger Bands
./build/examples/mean_reversion_strategy

# Complete multi-indicator strategy
./build/examples/complete_backtest

# With custom data
./build/examples/complete_backtest data/historical/AAPL.csv
```

## Generate Sample Data

```bash
python3 scripts/generate_sample_data.py
```

## Performance

- **Tick processing**: < 1μs latency
- **Order execution**: < 100ns
- **Indicator updates**: < 50ns (SMA/EMA)
- **Memory footprint**: ~100MB for 1M bars

## Technical Indicators

### Trend
- SMA (Simple Moving Average)
- EMA (Exponential Moving Average)
- WMA (Weighted Moving Average)

### Momentum
- RSI (Relative Strength Index)
- MACD (Moving Average Convergence Divergence)
- Stochastic Oscillator

### Volatility
- Bollinger Bands
- ATR (Average True Range)
- Keltner Channels

### Volume
- OBV (On-Balance Volume)
- VWAP (Volume Weighted Average Price)

## License

MIT License - See LICENSE file for details
