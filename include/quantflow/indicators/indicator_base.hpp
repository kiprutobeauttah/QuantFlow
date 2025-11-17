#pragma once

#include "quantflow/core/types.hpp"
#include <vector>
#include <deque>

namespace quantflow {
namespace indicators {

class Indicator {
public:
    virtual ~Indicator() = default;
    virtual void update(double value) = 0;
    virtual double value() const = 0;
    virtual bool is_ready() const = 0;
    virtual void reset() = 0;
};

} // namespace indicators
} // namespace quantflow
