#pragma once

#include "quantflow/core/types.hpp"
#include <vector>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace quantflow {
namespace data {

class ITimeSeriesDB {
public:
    virtual ~ITimeSeriesDB() = default;
    
    virtual void write_tick(const Tick& tick) = 0;
    virtual void write_bar(const Bar& bar) = 0;
    virtual void write_batch(const std::vector<Bar>& bars) = 0;
    
    virtual std::vector<Bar> read_bars(
        const Symbol& symbol,
        Timestamp start,
        Timestamp end
    ) = 0;
    
    virtual std::optional<Bar> read_latest_bar(const Symbol& symbol) = 0;
    
    virtual std::vector<Symbol> list_symbols() = 0;
    virtual Timestamp get_first_timestamp(const Symbol& symbol) = 0;
    virtual Timestamp get_last_timestamp(const Symbol& symbol) = 0;
    
    virtual void compact() = 0;
    virtual size_t get_size_bytes() = 0;
};

class MemoryTimeSeriesDB : public ITimeSeriesDB {
public:
    MemoryTimeSeriesDB() = default;
    
    void write_tick(const Tick& tick) override;
    void write_bar(const Bar& bar) override;
    void write_batch(const std::vector<Bar>& bars) override;
    
    std::vector<Bar> read_bars(
        const Symbol& symbol,
        Timestamp start,
        Timestamp end
    ) override;
    
    std::optional<Bar> read_latest_bar(const Symbol& symbol) override;
    
    std::vector<Symbol> list_symbols() override;
    Timestamp get_first_timestamp(const Symbol& symbol) override;
    Timestamp get_last_timestamp(const Symbol& symbol) override;
    
    void compact() override;
    size_t get_size_bytes() override;
    
    void clear();
    size_t get_bar_count(const Symbol& symbol) const;

private:
    std::unordered_map<Symbol, std::vector<Bar>> data_;
    mutable std::mutex mutex_;
    
    std::pair<size_t, size_t> find_range(
        const std::vector<Bar>& bars,
        Timestamp start,
        Timestamp end
    ) const;
};

} // namespace data
} // namespace quantflow
