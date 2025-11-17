#pragma once

#include "quantflow/core/types.hpp"
#include "quantflow/strategy/strategy_base.hpp"
#include <memory>
#include <vector>
#include <map>

namespace quantflow {
namespace backtest {

struct BacktestConfig {
    double initial_cash = 100000.0;
    double commission_rate = 0.001;
    double slippage_bps = 5.0;
    Timestamp start_time = 0;
    Timestamp end_time = 0;
};

struct BacktestResult {
    double total_return;
    double sharpe_ratio;
    double max_drawdown;
    int total_trades;
    int winning_trades;
    int losing_trades;
    double win_rate;
    double profit_factor;
    double final_equity;
};

class BacktestEngine : public strategy::StrategyContext {
public:
    explicit BacktestEngine(const BacktestConfig& config);
    
    void add_strategy(std::shared_ptr<strategy::Strategy> strategy);
    void add_data(const std::vector<Bar>& bars);
    
    void run();
    BacktestResult get_results() const;
    
    // StrategyContext interface
    OrderID buy(const Symbol& symbol, double quantity, double price = 0.0) override;
    OrderID sell(const Symbol& symbol, double quantity, double price = 0.0) override;
    void cancel_order(OrderID order_id) override;
    
    const Position* get_position(const Symbol& symbol) const override;
    const PortfolioState& get_portfolio() const override;
    double get_cash() const override;

private:
    BacktestConfig config_;
    PortfolioState portfolio_;
    std::vector<std::shared_ptr<strategy::Strategy>> strategies_;
    std::vector<Bar> bars_;
    std::map<OrderID, Order> orders_;
    OrderID next_order_id_;
    
    void process_bar(const Bar& bar);
    void execute_order(Order& order, double price);
    void update_portfolio(const Bar& bar);
};

} // namespace backtest
} // namespace quantflow
