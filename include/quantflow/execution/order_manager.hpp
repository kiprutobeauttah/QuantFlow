#pragma once

#include "quantflow/core/types.hpp"
#include <unordered_map>
#include <functional>
#include <mutex>

namespace quantflow {
namespace execution {

using OrderUpdateCallback = std::function<void(const Order&)>;
using FillCallback = std::function<void(const Fill&)>;

class OrderManager {
public:
    OrderManager();
    
    OrderID submit_order(Order order);
    void cancel_order(OrderID order_id);
    void modify_order(OrderID order_id, double new_price, double new_quantity);
    
    void update_order(const Order& order);
    void add_fill(const Fill& fill);
    
    const Order* get_order(OrderID order_id) const;
    std::vector<Order> get_open_orders() const;
    std::vector<Order> get_orders_by_symbol(const Symbol& symbol) const;
    
    void on_order_update(OrderUpdateCallback callback);
    void on_fill(FillCallback callback);
    
    size_t num_open_orders() const;
    size_t num_total_orders() const;

private:
    std::unordered_map<OrderID, Order> orders_;
    std::vector<Fill> fills_;
    
    OrderUpdateCallback order_callback_;
    FillCallback fill_callback_;
    
    mutable std::mutex mutex_;
    OrderID next_order_id_;
};

} // namespace execution
} // namespace quantflow
