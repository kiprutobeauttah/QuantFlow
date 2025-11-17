#include "quantflow/data/timeseries_db.hpp"
#include <algorithm>
#include <iterator>
#ifdef _WIN32
#include <mutex>
// Windows doesn't have shared_mutex in older MSVC, use regular mutex
#define std::shared_mutex std::mutex
#define std::shared_lock std::lock_guard
#define std::unique_lock std::lock_guard
#else
#include <shared_mutex>
#endif

namespace quantflow {
namespace data {

void MemoryTimeSeriesDB::write_tick(const Tick& tick) {
    Bar bar;
    bar.symbol = tick.symbol;
    bar.timestamp = tick.timestamp;
    bar.open = tick.last;
    bar.high = tick.last;
    bar.low = tick.last;
    bar.close = tick.last;
    bar.volume = tick.volume;
    bar.period = constants::NANOSECONDS_PER_SECOND;
    
    write_bar(bar);
}

void MemoryTimeSeriesDB::write_bar(const Bar& bar) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& bars = data_[bar.symbol];
    
    auto it = std::lower_bound(bars.begin(), bars.end(), bar,
        [](const Bar& a, const Bar& b) {
            return a.timestamp < b.timestamp;
        });
    
    if (it != bars.end() && it->timestamp == bar.timestamp) {
        *it = bar;
    } else {
        bars.insert(it, bar);
    }
}

void MemoryTimeSeriesDB::write_batch(const std::vector<Bar>& bars) {
    if (bars.empty()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_map<Symbol, std::vector<Bar>> grouped;
    for (const auto& bar : bars) {
        grouped[bar.symbol].push_back(bar);
    }
    
    for (auto& [symbol, symbol_bars] : grouped) {
        std::sort(symbol_bars.begin(), symbol_bars.end(),
            [](const Bar& a, const Bar& b) {
                return a.timestamp < b.timestamp;
            });
        
        auto& existing = data_[symbol];
        
        std::vector<Bar> merged;
        merged.reserve(existing.size() + symbol_bars.size());
        
        std::merge(
            existing.begin(), existing.end(),
            symbol_bars.begin(), symbol_bars.end(),
            std::back_inserter(merged),
            [](const Bar& a, const Bar& b) -> bool {
                return a.timestamp < b.timestamp;
            }
        );
        
        auto last = std::unique(merged.begin(), merged.end(),
            [](const Bar& a, const Bar& b) {
                return a.timestamp == b.timestamp;
            });
        merged.erase(last, merged.end());
        
        existing = std::move(merged);
    }
}

std::vector<Bar> MemoryTimeSeriesDB::read_bars(
    const Symbol& symbol,
    Timestamp start,
    Timestamp end) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(symbol);
    if (it == data_.end()) {
        return {};
    }
    
    const auto& bars = it->second;
    auto [start_idx, end_idx] = find_range(bars, start, end);
    
    if (start_idx >= bars.size()) {
        return {};
    }
    
    return std::vector<Bar>(
        bars.begin() + start_idx,
        bars.begin() + std::min(end_idx, bars.size())
    );
}

std::optional<Bar> MemoryTimeSeriesDB::read_latest_bar(const Symbol& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(symbol);
    if (it == data_.end() || it->second.empty()) {
        return std::nullopt;
    }
    
    return it->second.back();
}

std::vector<Symbol> MemoryTimeSeriesDB::list_symbols() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Symbol> symbols;
    symbols.reserve(data_.size());
    
    for (const auto& [symbol, _] : data_) {
        symbols.push_back(symbol);
    }
    
    return symbols;
}

Timestamp MemoryTimeSeriesDB::get_first_timestamp(const Symbol& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(symbol);
    if (it == data_.end() || it->second.empty()) {
        return 0;
    }
    
    return it->second.front().timestamp;
}

Timestamp MemoryTimeSeriesDB::get_last_timestamp(const Symbol& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(symbol);
    if (it == data_.end() || it->second.empty()) {
        return 0;
    }
    
    return it->second.back().timestamp;
}

void MemoryTimeSeriesDB::compact() {
    // No-op for memory database
}

size_t MemoryTimeSeriesDB::get_size_bytes() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t total = 0;
    for (const auto& [symbol, bars] : data_) {
        total += bars.size() * sizeof(Bar);
    }
    
    return total;
}

void MemoryTimeSeriesDB::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
}

size_t MemoryTimeSeriesDB::get_bar_count(const Symbol& symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = data_.find(symbol);
    if (it == data_.end()) {
        return 0;
    }
    
    return it->second.size();
}

std::pair<size_t, size_t> MemoryTimeSeriesDB::find_range(
    const std::vector<Bar>& bars,
    Timestamp start,
    Timestamp end) const {
    
    auto start_it = std::lower_bound(bars.begin(), bars.end(), start,
        [](const Bar& bar, Timestamp ts) {
            return bar.timestamp < ts;
        });
    
    auto end_it = std::upper_bound(bars.begin(), bars.end(), end,
        [](Timestamp ts, const Bar& bar) {
            return ts < bar.timestamp;
        });
    
    return {
        std::distance(bars.begin(), start_it),
        std::distance(bars.begin(), end_it)
    };
}

} // namespace data
} // namespace quantflow
