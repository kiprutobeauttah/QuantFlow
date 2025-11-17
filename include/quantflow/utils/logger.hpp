#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>

namespace quantflow {
namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    void set_level(LogLevel level) {
        level_ = level;
    }
    
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args) {
        if (level < level_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "[" << level_to_string(level) << "] ";
        print_formatted(format, std::forward<Args>(args)...);
        std::cout << std::endl;
    }

private:
    Logger() : level_(LogLevel::INFO) {}
    
    LogLevel level_;
    std::mutex mutex_;
    
    std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
    
    void print_formatted(const std::string& format) {
        std::cout << format;
    }
    
    template<typename T, typename... Args>
    void print_formatted(const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            std::cout << format.substr(0, pos) << value;
            print_formatted(format.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            std::cout << format;
        }
    }
};

#define LOG_DEBUG(...) quantflow::utils::Logger::instance().log(quantflow::utils::LogLevel::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) quantflow::utils::Logger::instance().log(quantflow::utils::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARN(...) quantflow::utils::Logger::instance().log(quantflow::utils::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR(...) quantflow::utils::Logger::instance().log(quantflow::utils::LogLevel::ERROR, __VA_ARGS__)

} // namespace utils
} // namespace quantflow
