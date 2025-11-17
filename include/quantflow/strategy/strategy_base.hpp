#pragma once

#include "quantflow/core/types.hpp"
#include <memory>
#include <vector>

namespace quantflow {
namespace strategy {

class StrategyContext;

class Strategy {
public:
    virtual ~Strategy() = default;
    
    virtual void on_init() {}
    virtual void on_tick(const Tick& tick) {}
    virtual void on_bar(const Bar& bar) = 0;
    virtual void on_order_update(const Order& order) {}
    virtual void on_fill(const Fill& fill) {}
    
    void set_context(StrategyContext* ctx) { context_ = ctx; }
    
protected:
    StrategyContext* context_ = nullptr;
};

class StrategyContext {
public:
    virtual ~StrategyContext() = default;
    
    virtual OrderID buy(const Symbol& symbol, double quantity, double price = 0.0) = 0;
    virtual OrderID sell(const Symbol& symbol, double quantity, double price = 0.0) = 0;
    virtual void cancel_order(OrderID order_id) = 0;
    
    virtual const Position* get_position(const Symbol& symbol) const = 0;
    virtual const PortfolioState& get_portfolio() const = 0;
    virtual double get_cash() const = 0;
};

} // namespace strategy
} // namespace quantflow
