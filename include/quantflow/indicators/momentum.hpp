#pragma once

#include "indicator_base.hpp"
#include "moving_average.hpp"
#include <deque>
#include <algorithm>

namespace quantflow {
namespace indicators {

class RSI : public Indicator {
public:
    explicit RSI(int period = 14)
        : period_(period),
          avg_gain_(0.0),
          avg_loss_(0.0),
          prev_close_(0.0),
          count_(0) {}
    
    void update(double value) override {
        if (count_ == 0) {
            prev_close_ = value;
            count_++;
            return;
        }
        
        double change = value - prev_close_;
        double gain = (change > 0) ? change : 0.0;
        double loss = (change < 0) ? -change : 0.0;
        
        if (count_ <= period_) {
            avg_gain_ += gain;
            avg_loss_ += loss;
        } else {
            avg_gain_ = (avg_gain_ * (period_ - 1) + gain) / period_;
            avg_loss_ = (avg_loss_ * (period_ - 1) + loss) / period_;
        }
        
        prev_close_ = value;
        count_++;
    }
    
    double value() const override {
        if (!is_ready() || avg_loss_ == 0.0) return 50.0;
        double rs = avg_gain_ / avg_loss_;
        return 100.0 - (100.0 / (1.0 + rs));
    }
    
    bool is_ready() const override {
        return count_ > period_;
    }
    
    void reset() override {
        avg_gain_ = 0.0;
        avg_loss_ = 0.0;
        prev_close_ = 0.0;
        count_ = 0;
    }

private:
    int period_;
    double avg_gain_;
    double avg_loss_;
    double prev_close_;
    int count_;
};

class MACD : public Indicator {
public:
    MACD(int fast = 12, int slow = 26, int signal = 9)
        : fast_ema_(fast),
          slow_ema_(slow),
          signal_ema_(signal),
          initialized_(false) {}
    
    void update(double value) override {
        fast_ema_.update(value);
        slow_ema_.update(value);
        
        if (fast_ema_.is_ready() && slow_ema_.is_ready()) {
            double macd_line = fast_ema_.value() - slow_ema_.value();
            signal_ema_.update(macd_line);
            initialized_ = true;
        }
    }
    
    double value() const override {
        return macd_line();
    }
    
    double macd_line() const {
        if (!fast_ema_.is_ready() || !slow_ema_.is_ready()) return 0.0;
        return fast_ema_.value() - slow_ema_.value();
    }
    
    double signal_line() const {
        return signal_ema_.value();
    }
    
    double histogram() const {
        return macd_line() - signal_line();
    }
    
    bool is_ready() const override {
        return initialized_ && signal_ema_.is_ready();
    }
    
    void reset() override {
        fast_ema_.reset();
        slow_ema_.reset();
        signal_ema_.reset();
        initialized_ = false;
    }

private:
    EMA fast_ema_;
    EMA slow_ema_;
    EMA signal_ema_;
    bool initialized_;
};

} // namespace indicators
} // namespace quantflow
