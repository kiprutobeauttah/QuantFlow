#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <limits>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace quantflow {

// Basic types
using Symbol = std::string;
using OrderID = uint64_t;
using FillID = uint64_t;
using StrategyID = std::string;
using ExchangeID = std::string;

// Time types (nanosecond precision)
using Timestamp = int64_t;
using Duration = int64_t;

// Price structure
struct Price {
    int64_t value;
    
    explicit Price(double price, int precision = 2)
        : value(static_cast<int64_t>(price * std::pow(10, precision))) {}
    
    double to_double() const { return value / 100.0; }
    
    Price operator+(Price other) const { return Price{value + other.value}; }
    Price operator-(Price other) const { return Price{value - other.value}; }
    Price operator*(double scalar) const { return Price{static_cast<int64_t>(value * scalar)}; }
    
    bool operator<(Price other) const { return value < other.value; }
    bool operator>(Price other) const { return value > other.value; }
    bool operator==(Price other) const { return value == other.value; }

private:
    explicit Price(int64_t v) : value(v) {}
};

using Quantity = double;

// Tick data
struct Tick {
    Symbol symbol;
    Timestamp timestamp;
    double last;
    double bid;
    double ask;
    uint64_t volume;
    uint32_t bid_size;
    uint32_t ask_size;
    uint8_t exchange_id;
    
    double mid() const { return (bid + ask) / 2.0; }
    double spread() const { return ask - bid; }
    double spread_bps() const { return (spread() / mid()) * 10000.0; }
};

// OHLCV bar
struct Bar {
    Symbol symbol;
    Timestamp timestamp;
    double open;
    double high;
    double low;
    double close;
    uint64_t volume;
    Duration period;
    
    double typical_price() const { return (high + low + close) / 3.0; }
    double hl_range() const { return high - low; }
    bool is_bullish() const { return close > open; }
};

// Order book level
struct OrderBookLevel {
    double price;
    uint64_t quantity;
    uint32_t num_orders;
    
    double notional() const { return price * quantity; }
};

// Order book
struct OrderBook {
    Symbol symbol;
    Timestamp timestamp;
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
    
    double mid_price() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return (bids[0].price + asks[0].price) / 2.0;
    }
    
    double spread() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return asks[0].price - bids[0].price;
    }
};

// Order types
enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT
};

enum class OrderSide {
    BUY,
    SELL,
    SHORT,
    COVER
};

enum class OrderStatus {
    PENDING,
    SUBMITTED,
    ACCEPTED,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED,
    EXPIRED
};

enum class TimeInForce {
    DAY,
    GTC,
    IOC,
    FOK
};

// Order structure
struct Order {
    OrderID id;
    Symbol symbol;
    OrderType type;
    OrderSide side;
    double quantity;
    double price;
    double stop_price;
    TimeInForce tif;
    OrderStatus status;
    
    double filled_quantity;
    double remaining_quantity;
    double avg_fill_price;
    
    Timestamp created_at;
    Timestamp submitted_at;
    Timestamp updated_at;
    Timestamp filled_at;
    
    StrategyID strategy_id;
    ExchangeID exchange_id;
    
    std::string client_order_id;
    std::string exchange_order_id;
    std::string rejection_reason;
    
    bool is_buy() const { return side == OrderSide::BUY || side == OrderSide::COVER; }
    bool is_sell() const { return side == OrderSide::SELL || side == OrderSide::SHORT; }
    bool is_filled() const { return status == OrderStatus::FILLED; }
    bool is_open() const {
        return status == OrderStatus::SUBMITTED ||
               status == OrderStatus::ACCEPTED ||
               status == OrderStatus::PARTIALLY_FILLED;
    }
};

// Fill structure
struct Fill {
    FillID id;
    OrderID order_id;
    Symbol symbol;
    OrderSide side;
    double quantity;
    double price;
    double commission;
    double slippage;
    Timestamp timestamp;
    ExchangeID exchange_id;
    
    double notional() const { return quantity * price; }
    double total_cost() const { return notional() + commission; }
};

// Position structure
struct Position {
    Symbol symbol;
    double quantity;
    double avg_entry_price;
    double current_price;
    
    double realized_pnl;
    double unrealized_pnl;
    double total_pnl;
    double total_commission;
    
    Timestamp opened_at;
    Timestamp last_updated;
    
    bool is_long() const { return quantity > 0; }
    bool is_short() const { return quantity < 0; }
    bool is_flat() const { return quantity == 0; }
    
    double market_value() const { return quantity * current_price; }
    double cost_basis() const { return quantity * avg_entry_price; }
};

// Portfolio state
struct PortfolioState {
    double cash;
    double equity;
    double margin_used;
    double margin_available;
    double buying_power;
    
    std::unordered_map<Symbol, Position> positions;
    Timestamp last_updated;
    
    double total_value() const { return equity; }
    
    int num_positions() const {
        return std::count_if(positions.begin(), positions.end(),
            [](const auto& p) { return !p.second.is_flat(); });
    }
};

// Event types
enum class EventType {
    TICK,
    BAR,
    ORDER_UPDATE,
    FILL,
    POSITION_UPDATE,
    TIMER,
    SYSTEM
};

struct Event {
    EventType type;
    Timestamp timestamp;
    void* data;
    
    bool operator<(const Event& other) const {
        return timestamp < other.timestamp;
    }
};

// Constants
namespace constants {
    constexpr double EPSILON = 1e-9;
    constexpr int64_t NANOSECONDS_PER_SECOND = 1'000'000'000LL;
    constexpr int TRADING_DAYS_PER_YEAR = 252;
    constexpr double RISK_FREE_RATE = 0.02;
}

} // namespace quantflow
