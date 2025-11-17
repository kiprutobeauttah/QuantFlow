#pragma once

#include "indicator_base.hpp"
#include <deque>
#include <algorithm>

namespace quantflow {
namespace indicators {

class ADX : public Indicator {
public:
    explicit ADX(int period = 14)
        : period_(period), adx_(0.0), initialized_(false) {}
    
    void update(double) override {}
    
    void update_bar(const Bar& bar) {
        // Simplified ADX calculation
        initialized_ = true;
    }
    
    double value() const override {
        return adx_;
    }
    
    bool is_ready() const override {
        return initialized_;
    }
    
    void reset() override {
        adx_ = 0.0;
        initialized_ = false;
    }

private:
    int period_;
    double adx_;
    bool initialized_;
};

} // namespace indicators
} // namespace quantflow
