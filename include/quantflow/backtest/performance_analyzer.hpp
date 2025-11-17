#pragma once

#include "quantflow/core/types.hpp"
#include <vector>
#include <cmath>

namespace quantflow {
namespace backtest {

struct PerformanceMetrics {
    double total_return;
    double annualized_return;
    double sharpe_ratio;
    double sortino_ratio;
    double max_drawdown;
    double max_drawdown_duration;
    
    int total_trades;
    int winning_trades;
    int losing_trades;
    double win_rate;
    
    double avg_win;
    double avg_loss;
    double profit_factor;
    double expectancy;
    
    double total_commission;
    double total_slippage;
};

class PerformanceAnalyzer {
public:
    static PerformanceMetrics calculate(
        const std::vector<double>& equity_curve,
        const std::vector<Fill>& fills,
        double initial_capital,
        double risk_free_rate = 0.02
    );
    
private:
    static double calculate_sharpe_ratio(
        const std::vector<double>& returns,
        double risk_free_rate
    );
    
    static double calculate_max_drawdown(
        const std::vector<double>& equity_curve
    );
};

} // namespace backtest
} // namespace quantflow
