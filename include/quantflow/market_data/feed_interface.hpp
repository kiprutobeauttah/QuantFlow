#pragma once

#include "quantflow/core/types.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace quantflow {
namespace market_data {

using TickCallback = std::function<void(const Tick&)>;
using BarCallback = std::function<void(const Bar&)>;
using OrderBookCallback = std::function<void(const OrderBook&)>;

class IMarketDataFeed {
public:
    virtual ~IMarketDataFeed() = default;
    
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    virtual void subscribe(const Symbol& symbol) = 0;
    virtual void unsubscribe(const Symbol& symbol) = 0;
    virtual void subscribe_all() = 0;
    
    virtual void on_tick(TickCallback callback) = 0;
    virtual void on_bar(BarCallback callback) = 0;
    virtual void on_orderbook(OrderBookCallback callback) = 0;
    
    virtual void start() = 0;
    virtual void stop() = 0;
    
    virtual size_t num_subscriptions() const = 0;
    virtual std::vector<Symbol> subscribed_symbols() const = 0;
};

struct LiveFeedConfig {
    std::string endpoint;
    std::string api_key;
    std::string api_secret;
    
    bool use_orderbook = false;
    bool use_trades = true;
    bool use_bars = false;
    
    Duration bar_period = 60 * constants::NANOSECONDS_PER_SECOND;
    size_t reconnect_attempts = 5;
    Duration reconnect_delay = 5 * constants::NANOSECONDS_PER_SECOND;
    
    size_t buffer_size = 8192;
    bool enable_compression = false;
};

struct HistoricalFeedConfig {
    std::string data_directory;
    Timestamp start_date;
    Timestamp end_date;
    
    double replay_speed = 0.0;
    bool loop = false;
    
    size_t cache_size_mb = 512;
    bool preload_all = false;
};

} // namespace market_data
} // namespace quantflow
