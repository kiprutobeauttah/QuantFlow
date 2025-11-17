#pragma once

#include "indicator_base.hpp"
#include "moving_average.hpp"
#include <cmath>

namespace quantflow {
namespace indicators {

class BollingerBands : public Indicator {
public:
    BollingerBands(int period = 20, double num_std = 2.0)
        : period_(period),
          num_std_(num_std),
          sma_(period) {}
    
    void update(double value) override {
        values_.push_back(value);
        sma_.update(value);
        
        if (values_.size() > static_cast<size_t>(period_)) {
            values_.pop_front();
        }
    }
    
    double value() const override {
        return sma_.value();
    }
    
    double upper_band() const {
        return sma_.value() + num_std_ * std_dev();
    }
    
    double lower_band() const {
        return sma_.value() - num_std_ * std_dev();
    }
    
    bool is_ready() const override {
        return sma_.is_ready();
    }
    
    void reset() override {
        values_.clear();
        sma_.reset();
    }

private:
    int period_;
    double num_std_;
    SMA sma_;
    std::deque<double> values_;
    
    double std_dev() const {
        if (values_.size() < 2) return 0.0;
        
        double mean = sma_.value();
        double sum_sq = 0.0;
        
        for (double val : values_) {
            double diff = val - mean;
            sum_sq += diff * diff;
        }
        
        return std::sqrt(sum_sq / values_.size());
    }
};

class ATR : public Indicator {
public:
    explicit ATR(int period = 14)
        : period_(period),
          atr_(0.0),
          prev_close_(0.0),
          initialized_(false) {}
    
    void update_bar(const Bar& bar) {
        double tr = true_range(bar);
        
        if (!initialized_) {
            atr_ = tr;
            initialized_ = true;
        } else {
            atr_ = ((period_ - 1) * atr_ + tr) / period_;
        }
        
        prev_close_ = bar.close;
    }
    
    void update(double) override {}
    
    double value() const override {
        return atr_;
    }
    
    bool is_ready() const override {
        return initialized_;
    }
    
    void reset() override {
        atr_ = 0.0;
        prev_close_ = 0.0;
        initialized_ = false;
    }

private:
    int period_;
    double atr_;
    double prev_close_;
    bool initialized_;
    
    double true_range(const Bar& bar) const {
        if (!initialized_) {
            return bar.high - bar.low;
        }
        
        double hl = bar.high - bar.low;
        double hc = std::abs(bar.high - prev_close_);
        double lc = std::abs(bar.low - prev_close_);
        
        return std::max({hl, hc, lc});
    }
};

} // namespace indicators
} // namespace quantflow
