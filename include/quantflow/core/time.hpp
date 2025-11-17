#pragma once

#include "types.hpp"
#include <string>

namespace quantflow {

class TimeUtils {
public:
    static Timestamp now() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    static std::string to_string(Timestamp ts);
    static Timestamp from_string(const std::string& iso8601);
    static bool is_market_hours(Timestamp ts);
};

} // namespace quantflow
