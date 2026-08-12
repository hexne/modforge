/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/12
********************************************************************************/
module;
export module modforge.lock_free_queue;

import std;

export template <typename T>
class SPMCQueue {
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    std::size_t capacity_{};

public:
    explicit SPMCQueue(std::size_t capacity = 1024)
        : capacity_(capacity) {}

    bool push(const T& value) {
        {
            std::lock_guard lock(mutex_);
            if (queue_.size() >= capacity_) {
                return false;
            }
            queue_.push(value);
        }
        cv_.notify_one();
        return true;
    }

    bool push(T&& value) {
        {
            std::lock_guard lock(mutex_);
            if (queue_.size() >= capacity_) {
                return false;
            }
            queue_.push(std::move(value));
        }
        cv_.notify_one();
        return true;
    }

    std::optional<T> try_pop() {
        std::lock_guard lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }
};
