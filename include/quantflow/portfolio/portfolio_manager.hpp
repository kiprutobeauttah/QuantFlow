#pragma once

#include "quantflow/core/types.hpp"
#include <unordered_map>

namespace quantflow {
namespace portfolio {

class PortfolioManager {
public:
    explicit PortfolioManager(double initial_cash)
        : state_{} {
        state_.cash = initial_cash;
        state_.equity = initial_cash;
        state_.buying_power = initial_cash;
    }
    
    void update_position(const Fill& fill) {
        auto& pos = state_.positions[fill.symbol];
        
        if (fill.side == OrderSide::BUY) {
            double new_qty = pos.quantity + fill.quantity;
            pos.avg_entry_price = ((pos.avg_entry_price * pos.quantity) +
                                   (fill.price * fill.quantity)) / new_qty;
            pos.quantity = new_qty;
            state_.cash -= fill.total_cost();
        } else {
            pos.quantity -= fill.quantity;
            double pnl = (fill.price - pos.avg_entry_price) * fill.quantity;
            pos.realized_pnl += pnl;
            state_.cash += fill.notional() - fill.commission;
        }
        
        pos.total_commission += fill.commission;
    }
    
    void update_prices(const std::unordered_map<Symbol, double>& prices) {
        for (auto& [symbol, pos] : state_.positions) {
            auto it = prices.find(symbol);
            if (it != prices.end()) {
                pos.current_price = it->second;
                pos.unrealized_pnl = (pos.current_price - pos.avg_entry_price) * pos.quantity;
            }
        }
        
        state_.equity = state_.cash;
        for (const auto& [sym, pos] : state_.positions) {
            state_.equity += pos.market_value();
        }
    }
    
    const PortfolioState& get_state() const { return state_; }
    const Position* get_position(const Symbol& symbol) const {
        auto it = state_.positions.find(symbol);
        return (it != state_.positions.end()) ? &it->second : nullptr;
    }

private:
    PortfolioState state_;
};

} // namespace portfolio
} // namespace quantflow
