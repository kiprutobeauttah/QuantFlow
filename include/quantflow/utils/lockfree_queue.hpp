#pragma once

#include <atomic>
#include <cassert>

namespace quantflow {
namespace utils {

template<typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity)
        : capacity_(capacity),
          buffer_(new T[capacity]),
          head_(0),
          tail_(0) {
        assert((capacity & (capacity - 1)) == 0);
    }
    
    ~SPSCQueue() {
        delete[] buffer_;
    }
    
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    
    bool push(const T& item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = (current_tail + 1) & (capacity_ - 1);
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        
        item = std::move(buffer_[current_head]);
        head_.store((current_head + 1) & (capacity_ - 1), std::memory_order_release);
        return true;
    }
    
    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

private:
    const size_t capacity_;
    T* buffer_;
    
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};

} // namespace utils
} // namespace quantflow
