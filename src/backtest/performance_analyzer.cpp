#include "quantflow/backtest/performance_analyzer.hpp"
#include <numeric>
#include <algorithm>

namespace quantflow {
namespace backtest {

PerformanceMetrics PerformanceAnalyzer::calculate(
    const std::vector<double>& equity_curve,
    const std::vector<Fill>& fills,
    double initial_capital,
    double risk_free_rate) {
    
    PerformanceMetrics metrics{};
    
    if (equity_curve.empty()) {
        return metrics;
    }
    
    double final_equity = equity_curve.back();
    metrics.total_return = ((final_equity - initial_capital) / initial_capital) * 100.0;
    
    std::vector<double> returns;
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double ret = (equity_curve[i] - equity_curve[i-1]) / equity_curve[i-1];
        returns.push_back(ret);
    }
    
    metrics.sharpe_ratio = calculate_sharpe_ratio(returns, risk_free_rate);
    metrics.max_drawdown = calculate_max_drawdown(equity_curve);
    
    metrics.total_trades = fills.size();
    
    double total_pnl = 0.0;
    double total_wins = 0.0;
    double total_losses = 0.0;
    
    for (const auto& fill : fills) {
        double pnl = fill.notional();
        total_pnl += pnl;
        
        if (pnl > 0) {
            metrics.winning_trades++;
            total_wins += pnl;
        } else if (pnl < 0) {
            metrics.losing_trades++;
            total_losses += std::abs(pnl);
        }
        
        metrics.total_commission += fill.commission;
        metrics.total_slippage += fill.slippage;
    }
    
    metrics.win_rate = (metrics.total_trades > 0) ?
        (static_cast<double>(metrics.winning_trades) / metrics.total_trades) * 100.0 : 0.0;
    
    metrics.avg_win = (metrics.winning_trades > 0) ?
        total_wins / metrics.winning_trades : 0.0;
    
    metrics.avg_loss = (metrics.losing_trades > 0) ?
        total_losses / metrics.losing_trades : 0.0;
    
    metrics.profit_factor = (total_losses > 0) ?
        total_wins / total_losses : 0.0;
    
    return metrics;
}

double PerformanceAnalyzer::calculate_sharpe_ratio(
    const std::vector<double>& returns,
    double risk_free_rate) {
    
    if (returns.empty()) return 0.0;
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    
    double sq_sum = 0.0;
    for (double ret : returns) {
        sq_sum += (ret - mean) * (ret - mean);
    }
    double std_dev = std::sqrt(sq_sum / returns.size());
    
    if (std_dev < 1e-9) return 0.0;
    
    double daily_rf = risk_free_rate / 252.0;
    return ((mean - daily_rf) / std_dev) * std::sqrt(252.0);
}

double PerformanceAnalyzer::calculate_max_drawdown(
    const std::vector<double>& equity_curve) {
    
    if (equity_curve.empty()) return 0.0;
    
    double max_dd = 0.0;
    double peak = equity_curve[0];
    
    for (double equity : equity_curve) {
        if (equity > peak) {
            peak = equity;
        }
        
        double dd = (peak - equity) / peak;
        max_dd = std::max(max_dd, dd);
    }
    
    return max_dd * 100.0;
}

} // namespace backtest
} // namespace quantflow
