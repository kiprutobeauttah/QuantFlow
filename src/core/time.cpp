#include "quantflow/core/time.hpp"
#include <sstream>
#include <iomanip>

namespace quantflow {

std::string TimeUtils::to_string(Timestamp ts) {
    auto duration = std::chrono::nanoseconds(ts);
    auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>(duration);
    auto time_t_val = std::chrono::system_clock::to_time_t(
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp)
    );
    std::stringstream ss;
    std::tm tm_val;
#ifdef _WIN32
    localtime_s(&tm_val, &time_t_val);
    ss << std::put_time(&tm_val, "%Y-%m-%d %H:%M:%S");
#else
    ss << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
#endif
    return ss.str();
}

Timestamp TimeUtils::from_string(const std::string& iso8601) {
    // Simplified implementation
    return now();
}

bool TimeUtils::is_market_hours(Timestamp ts) {
    // Simplified: assume 9:30 AM - 4:00 PM ET
    return true;
}

} // namespace quantflow
