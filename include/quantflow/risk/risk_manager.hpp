#pragma once

#include "quantflow/core/types.hpp"

namespace quantflow {
namespace risk {

struct RiskLimits {
    double max_position_size = 10000.0;
    double max_portfolio_leverage = 2.0;
    double max_drawdown_pct = 20.0;
    double max_loss_per_trade = 500.0;
};

class RiskManager {
public:
    explicit RiskManager(const RiskLimits& limits)
        : limits_(limits) {}
    
    bool validate_order(const Order& order, const PortfolioState& portfolio) {
        // Check position size
        if (order.quantity * order.price > limits_.max_position_size) {
            return false;
        }
        
        // Check available cash
        if (order.is_buy()) {
            double cost = order.quantity * order.price;
            if (cost > portfolio.cash) {
                return false;
            }
        }
        
        return true;
    }
    
    double calculate_position_size(
        double account_value,
        double risk_per_trade,
        double entry_price,
        double stop_loss_price) {
        
        double risk_amount = account_value * risk_per_trade;
        double risk_per_share = std::abs(entry_price - stop_loss_price);
        
        if (risk_per_share < 1e-9) return 0.0;
        
        return risk_amount / risk_per_share;
    }

private:
    RiskLimits limits_;
};

} // namespace risk
} // namespace quantflow
