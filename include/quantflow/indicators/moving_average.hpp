#pragma once

#include "indicator_base.hpp"
#include <deque>
#include <numeric>

namespace quantflow {
namespace indicators {

class SMA : public Indicator {
public:
    explicit SMA(int period) : period_(period), sum_(0.0) {}
    
    void update(double value) override {
        values_.push_back(value);
        sum_ += value;
        
        if (values_.size() > static_cast<size_t>(period_)) {
            sum_ -= values_.front();
            values_.pop_front();
        }
    }
    
    double value() const override {
        if (!is_ready()) return 0.0;
        return sum_ / period_;
    }
    
    bool is_ready() const override {
        return values_.size() >= static_cast<size_t>(period_);
    }
    
    void reset() override {
        values_.clear();
        sum_ = 0.0;
    }

private:
    int period_;
    std::deque<double> values_;
    double sum_;
};

class EMA : public Indicator {
public:
    explicit EMA(int period)
        : period_(period),
          multiplier_(2.0 / (period + 1)),
          ema_(0.0),
          initialized_(false) {}
    
    void update(double value) override {
        if (!initialized_) {
            ema_ = value;
            initialized_ = true;
        } else {
            ema_ = (value - ema_) * multiplier_ + ema_;
        }
    }
    
    double value() const override {
        return ema_;
    }
    
    bool is_ready() const override {
        return initialized_;
    }
    
    void reset() override {
        ema_ = 0.0;
        initialized_ = false;
    }

private:
    int period_;
    double multiplier_;
    double ema_;
    bool initialized_;
};

} // namespace indicators
} // namespace quantflow
