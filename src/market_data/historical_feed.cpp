#include "quantflow/market_data/historical_feed.hpp"
#include "quantflow/core/time.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace quantflow {
namespace market_data {

HistoricalFeed::HistoricalFeed(const HistoricalFeedConfig& config)
    : config_(config),
      connected_(false),
      running_(false),
      current_time_(config.start_date),
      start_time_(config.start_date),
      end_time_(config.end_date) {}

HistoricalFeed::~HistoricalFeed() {
    stop();
    
    for (auto& [symbol, file] : data_files_) {
        if (file.file) {
            fclose(file.file);
        }
    }
}

void HistoricalFeed::connect() {
    connected_.store(true);
}

void HistoricalFeed::disconnect() {
    connected_.store(false);
}

void HistoricalFeed::subscribe(const Symbol& symbol) {
    load_data_file(symbol);
}

void HistoricalFeed::unsubscribe(const Symbol& symbol) {
    auto it = data_files_.find(symbol);
    if (it != data_files_.end()) {
        if (it->second.file) {
            fclose(it->second.file);
        }
        data_files_.erase(it);
    }
}

void HistoricalFeed::subscribe_all() {
    namespace fs = std::filesystem;
    
    for (const auto& entry : fs::directory_iterator(config_.data_directory)) {
        if (entry.path().extension() == ".csv") {
            Symbol symbol = entry.path().stem().string();
            subscribe(symbol);
        }
    }
}

void HistoricalFeed::on_tick(TickCallback callback) {
    tick_callback_ = callback;
}

void HistoricalFeed::on_bar(BarCallback callback) {
    bar_callback_ = callback;
}

void HistoricalFeed::on_orderbook(OrderBookCallback callback) {
    orderbook_callback_ = callback;
}

void HistoricalFeed::start() {
    if (running_.exchange(true)) {
        return;
    }
    
    for (const auto& [symbol, _] : data_files_) {
        read_next_event(symbol);
    }
    
    replay_thread_ = std::thread(&HistoricalFeed::replay_events, this);
}

void HistoricalFeed::stop() {
    running_.store(false);
    
    if (replay_thread_.joinable()) {
        replay_thread_.join();
    }
}

void HistoricalFeed::load_data_file(const Symbol& symbol) {
    std::string path = config_.data_directory + "/" + symbol + ".csv";
    
    FILE* file = fopen(path.c_str(), "r");
    if (!file) {
        throw std::runtime_error("Failed to open data file: " + path);
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char header[1024];
    fgets(header, sizeof(header), file);
    
    DataFile df;
    df.path = path;
    df.file = file;
    df.size = size;
    df.offset = ftell(file);
    df.eof = false;
    
    data_files_[symbol] = df;
}

bool HistoricalFeed::read_next_event(const Symbol& symbol) {
    auto it = data_files_.find(symbol);
    if (it == data_files_.end() || it->second.eof) {
        return false;
    }
    
    DataFile& df = it->second;
    char line[2048];
    
    if (!fgets(line, sizeof(line), df.file)) {
        df.eof = true;
        return false;
    }
    
    std::stringstream ss(line);
    std::string field;
    std::vector<std::string> fields;
    
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
    
    if (fields.size() < 6) {
        return read_next_event(symbol);
    }
    
    try {
        Bar bar;
        bar.symbol = symbol;
        bar.timestamp = std::stoll(fields[0]);
        bar.open = std::stod(fields[1]);
        bar.high = std::stod(fields[2]);
        bar.low = std::stod(fields[3]);
        bar.close = std::stod(fields[4]);
        bar.volume = std::stoull(fields[5]);
        bar.period = 60 * constants::NANOSECONDS_PER_SECOND;
        
        if (bar.timestamp >= start_time_ && bar.timestamp <= end_time_) {
            event_queue_.push({bar.timestamp, bar});
        }
        
        df.offset = ftell(df.file);
        return true;
        
    } catch (const std::exception&) {
        return read_next_event(symbol);
    }
}

void HistoricalFeed::replay_events() {
    auto replay_start = std::chrono::steady_clock::now();
    Timestamp sim_start_time = current_time_;
    
    while (running_.load() && !event_queue_.empty()) {
        auto [timestamp, event_variant] = event_queue_.top();
        event_queue_.pop();
        
        current_time_ = timestamp;
        
        if (config_.replay_speed > 0.0) {
            Timestamp sim_elapsed = current_time_ - sim_start_time;
            auto real_elapsed = std::chrono::steady_clock::now() - replay_start;
            auto real_elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(real_elapsed).count();
            
            Timestamp target_real_elapsed = sim_elapsed / config_.replay_speed;
            
            if (real_elapsed_ns < target_real_elapsed) {
                std::this_thread::sleep_for(
                    std::chrono::nanoseconds(target_real_elapsed - real_elapsed_ns)
                );
            }
        }
        
        std::visit([this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            
            if constexpr (std::is_same_v<T, Tick>) {
                if (tick_callback_) tick_callback_(arg);
            } else if constexpr (std::is_same_v<T, Bar>) {
                if (bar_callback_) bar_callback_(arg);
            } else if constexpr (std::is_same_v<T, OrderBook>) {
                if (orderbook_callback_) orderbook_callback_(arg);
            }
        }, event_variant);
        
        if (std::holds_alternative<Bar>(event_variant)) {
            const Bar& bar = std::get<Bar>(event_variant);
            read_next_event(bar.symbol);
        }
    }
    
    if (config_.loop && running_.load()) {
        seek(start_time_);
        replay_events();
    }
}

void HistoricalFeed::seek(Timestamp timestamp) {
    event_queue_ = decltype(event_queue_)();
    
    for (auto& [symbol, df] : data_files_) {
        fseek(df.file, 0, SEEK_SET);
        
        char header[1024];
        fgets(header, sizeof(header), df.file);
        
        df.offset = ftell(df.file);
        df.eof = false;
        
        while (read_next_event(symbol)) {
            if (!event_queue_.empty() && event_queue_.top().first >= timestamp) {
                break;
            }
        }
    }
    
    current_time_ = timestamp;
}

void HistoricalFeed::set_speed(double multiplier) {
    config_.replay_speed = multiplier;
}

double HistoricalFeed::get_progress() const {
    if (end_time_ <= start_time_) return 0.0;
    return static_cast<double>(current_time_ - start_time_) / (end_time_ - start_time_);
}

size_t HistoricalFeed::num_subscriptions() const {
    return data_files_.size();
}

std::vector<Symbol> HistoricalFeed::subscribed_symbols() const {
    std::vector<Symbol> symbols;
    for (const auto& [symbol, _] : data_files_) {
        symbols.push_back(symbol);
    }
    return symbols;
}

} // namespace market_data
} // namespace quantflow
