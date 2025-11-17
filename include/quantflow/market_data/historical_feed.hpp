#pragma once

#include "feed_interface.hpp"
#include <unordered_map>
#include <queue>
#include <variant>
#include <thread>
#include <atomic>

namespace quantflow {
namespace market_data {

class HistoricalFeed : public IMarketDataFeed {
public:
    explicit HistoricalFeed(const HistoricalFeedConfig& config);
    ~HistoricalFeed() override;
    
    void connect() override;
    void disconnect() override;
    bool is_connected() const override { return connected_.load(); }
    
    void subscribe(const Symbol& symbol) override;
    void unsubscribe(const Symbol& symbol) override;
    void subscribe_all() override;
    
    void on_tick(TickCallback callback) override;
    void on_bar(BarCallback callback) override;
    void on_orderbook(OrderBookCallback callback) override;
    
    void start() override;
    void stop() override;
    
    size_t num_subscriptions() const override;
    std::vector<Symbol> subscribed_symbols() const override;
    
    void seek(Timestamp timestamp);
    void set_speed(double multiplier);
    Timestamp current_time() const { return current_time_; }
    double get_progress() const;

private:
    struct DataFile {
        std::string path;
        FILE* file;
        size_t size;
        size_t offset;
        bool eof;
    };
    
    HistoricalFeedConfig config_;
    std::unordered_map<Symbol, DataFile> data_files_;
    
    TickCallback tick_callback_;
    BarCallback bar_callback_;
    OrderBookCallback orderbook_callback_;
    
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread replay_thread_;
    
    Timestamp current_time_;
    Timestamp start_time_;
    Timestamp end_time_;
    
    using TimedEvent = std::pair<Timestamp, std::variant<Tick, Bar, OrderBook>>;
    std::priority_queue<
        TimedEvent,
        std::vector<TimedEvent>,
        std::greater<TimedEvent>
    > event_queue_;
    
    void load_data_file(const Symbol& symbol);
    void replay_events();
    bool read_next_event(const Symbol& symbol);
};

} // namespace market_data
} // namespace quantflow
