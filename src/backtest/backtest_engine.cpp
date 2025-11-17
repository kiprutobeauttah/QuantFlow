#include "quantflow/backtest/backtest_engine.hpp"
#include <algorithm>
#include <cmath>

namespace quantflow {
namespace backtest {

BacktestEngine::BacktestEngine(const BacktestConfig& config)
    : config_(config), next_order_id_(1) {
    portfolio_.cash = config.initial_cash;
    portfolio_.equity = config.initial_cash;
    portfolio_.buying_power = config.initial_cash;
}

void BacktestEngine::add_strategy(std::shared_ptr<strategy::Strategy> strategy) {
    strategy->set_context(this);
    strategies_.push_back(strategy);
}

void BacktestEngine::add_data(const std::vector<Bar>& bars) {
    bars_ = bars;
    std::sort(bars_.begin(), bars_.end(),
        [](const Bar& a, const Bar& b) { return a.timestamp < b.timestamp; });
}

void BacktestEngine::run() {
    for (auto& strategy : strategies_) {
        strategy->on_init();
    }
    
    for (const auto& bar : bars_) {
        process_bar(bar);
    }
}

void BacktestEngine::process_bar(const Bar& bar) {
    update_portfolio(bar);
    
    for (auto& strategy : strategies_) {
        strategy->on_bar(bar);
    }
    
    // Execute pending orders
    for (auto& [id, order] : orders_) {
        if (order.is_open() && order.symbol == bar.symbol) {
            execute_order(order, bar.close);
        }
    }
}

void BacktestEngine::execute_order(Order& order, double price) {
    double fill_price = price * (1.0 + config_.slippage_bps / 10000.0);
    double commission = order.quantity * fill_price * config_.commission_rate;
    
    Fill fill;
    fill.id = next_order_id_++;
    fill.order_id = order.id;
    fill.symbol = order.symbol;
    fill.side = order.side;
    fill.quantity = order.quantity;
    fill.price = fill_price;
    fill.commission = commission;
    fill.timestamp = order.created_at;
    
    order.status = OrderStatus::FILLED;
    order.filled_quantity = order.quantity;
    order.avg_fill_price = fill_price;
    
    // Update position
    auto& pos = portfolio_.positions[order.symbol];
    if (order.is_buy()) {
        pos.quantity += order.quantity;
        pos.avg_entry_price = ((pos.avg_entry_price * (pos.quantity - order.quantity)) +
                               (fill_price * order.quantity)) / pos.quantity;
        portfolio_.cash -= fill.total_cost();
    } else {
        pos.quantity -= order.quantity;
        portfolio_.cash += fill.notional() - commission;
    }
    
    for (auto& strategy : strategies_) {
        strategy->on_fill(fill);
    }
}

void BacktestEngine::update_portfolio(const Bar& bar) {
    auto it = portfolio_.positions.find(bar.symbol);
    if (it != portfolio_.positions.end()) {
        it->second.current_price = bar.close;
        it->second.unrealized_pnl = (bar.close - it->second.avg_entry_price) * it->second.quantity;
    }
    
    double total_value = portfolio_.cash;
    for (const auto& [sym, pos] : portfolio_.positions) {
        total_value += pos.market_value();
    }
    portfolio_.equity = total_value;
}

OrderID BacktestEngine::buy(const Symbol& symbol, double quantity, double price) {
    Order order;
    order.id = next_order_id_++;
    order.symbol = symbol;
    order.type = (price > 0) ? OrderType::LIMIT : OrderType::MARKET;
    order.side = OrderSide::BUY;
    order.quantity = quantity;
    order.price = price;
    order.status = OrderStatus::SUBMITTED;
    order.created_at = TimeUtils::now();
    
    orders_[order.id] = order;
    return order.id;
}

OrderID BacktestEngine::sell(const Symbol& symbol, double quantity, double price) {
    Order order;
    order.id = next_order_id_++;
    order.symbol = symbol;
    order.type = (price > 0) ? OrderType::LIMIT : OrderType::MARKET;
    order.side = OrderSide::SELL;
    order.quantity = quantity;
    order.price = price;
    order.status = OrderStatus::SUBMITTED;
    order.created_at = TimeUtils::now();
    
    orders_[order.id] = order;
    return order.id;
}

void BacktestEngine::cancel_order(OrderID order_id) {
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        it->second.status = OrderStatus::CANCELLED;
    }
}

const Position* BacktestEngine::get_position(const Symbol& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    return (it != portfolio_.positions.end()) ? &it->second : nullptr;
}

const PortfolioState& BacktestEngine::get_portfolio() const {
    return portfolio_;
}

double BacktestEngine::get_cash() const {
    return portfolio_.cash;
}

BacktestResult BacktestEngine::get_results() const {
    BacktestResult result;
    result.final_equity = portfolio_.equity;
    result.total_return = ((portfolio_.equity - config_.initial_cash) / config_.initial_cash) * 100.0;
    result.total_trades = orders_.size();
    result.sharpe_ratio = 0.0;
    result.max_drawdown = 0.0;
    result.winning_trades = 0;
    result.losing_trades = 0;
    result.win_rate = 0.0;
    result.profit_factor = 0.0;
    return result;
}

} // namespace backtest
} // namespace quantflow
