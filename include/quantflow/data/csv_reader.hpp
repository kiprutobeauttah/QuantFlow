#pragma once

#include "quantflow/core/types.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace quantflow {
namespace data {

class CSVReader {
public:
    static std::vector<Bar> read_bars(const std::string& filename) {
        std::vector<Bar> bars;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            return bars;
        }
        
        std::string line;
        std::getline(file, line); // Skip header
        
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            Bar bar;
            
            std::getline(ss, token, ','); // timestamp
            bar.timestamp = std::stoll(token);
            
            std::getline(ss, token, ','); // symbol
            bar.symbol = token;
            
            std::getline(ss, token, ','); // open
            bar.open = std::stod(token);
            
            std::getline(ss, token, ','); // high
            bar.high = std::stod(token);
            
            std::getline(ss, token, ','); // low
            bar.low = std::stod(token);
            
            std::getline(ss, token, ','); // close
            bar.close = std::stod(token);
            
            std::getline(ss, token, ','); // volume
            bar.volume = std::stoull(token);
            
            bars.push_back(bar);
        }
        
        return bars;
    }
};

} // namespace data
} // namespace quantflow
