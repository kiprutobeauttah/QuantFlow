#include "quantflow/execution/order_manager.hpp"
#include "quantflow/core/time.hpp"

namespace quantflow {
namespace execution {

OrderManager::OrderManager() : next_order_id_(1) {}

OrderID OrderManager::submit_order(Order order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    order.id = next_order_id_++;
    order.created_at = TimeUtils::now();
    order.status = OrderStatus::SUBMITTED;
    order.filled_quantity = 0.0;
    order.remaining_quantity = order.quantity;
    
    orders_[order.id] = order;
    
    if (order_callback_) {
        order_callback_(order);
    }
    
    return order.id;
}

void OrderManager::cancel_order(OrderID order_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(order_id);
    if (it != orders_.end() && it->second.is_open()) {
        it->second.status = OrderStatus::CANCELLED;
        it->second.updated_at = TimeUtils::now();
        
        if (order_callback_) {
            order_callback_(it->second);
        }
    }
}

void OrderManager::modify_order(OrderID order_id, double new_price, double new_quantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(order_id);
    if (it != orders_.end() && it->second.is_open()) {
        it->second.price = new_price;
        it->second.quantity = new_quantity;
        it->second.remaining_quantity = new_quantity - it->second.filled_quantity;
        it->second.updated_at = TimeUtils::now();
        
        if (order_callback_) {
            order_callback_(it->second);
        }
    }
}

void OrderManager::update_order(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    orders_[order.id] = order;
    
    if (order_callback_) {
        order_callback_(order);
    }
}

void OrderManager::add_fill(const Fill& fill) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    fills_.push_back(fill);
    
    auto it = orders_.find(fill.order_id);
    if (it != orders_.end()) {
        Order& order = it->second;
        order.filled_quantity += fill.quantity;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        
        double total_value = order.avg_fill_price * (order.filled_quantity - fill.quantity) +
                            fill.price * fill.quantity;
        order.avg_fill_price = total_value / order.filled_quantity;
        
        if (order.remaining_quantity <= 0.0) {
            order.status = OrderStatus::FILLED;
            order.filled_at = fill.timestamp;
        } else {
            order.status = OrderStatus::PARTIALLY_FILLED;
        }
        
        order.updated_at = fill.timestamp;
        
        if (order_callback_) {
            order_callback_(order);
        }
    }
    
    if (fill_callback_) {
        fill_callback_(fill);
    }
}

const Order* OrderManager::get_order(OrderID order_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(order_id);
    return (it != orders_.end()) ? &it->second : nullptr;
}

std::vector<Order> OrderManager::get_open_orders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Order> open_orders;
    for (const auto& [_, order] : orders_) {
        if (order.is_open()) {
            open_orders.push_back(order);
        }
    }
    return open_orders;
}

std::vector<Order> OrderManager::get_orders_by_symbol(const Symbol& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Order> symbol_orders;
    for (const auto& [_, order] : orders_) {
        if (order.symbol == symbol) {
            symbol_orders.push_back(order);
        }
    }
    return symbol_orders;
}

void OrderManager::on_order_update(OrderUpdateCallback callback) {
    order_callback_ = callback;
}

void OrderManager::on_fill(FillCallback callback) {
    fill_callback_ = callback;
}

size_t OrderManager::num_open_orders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return std::count_if(orders_.begin(), orders_.end(),
        [](const auto& p) { return p.second.is_open(); });
}

size_t OrderManager::num_total_orders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_.size();
}

} // namespace execution
} // namespace quantflow
