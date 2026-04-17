/********************************************************************************
* @Author : hexne
* @Date   : 2026/04/17 18:15:55
********************************************************************************/

module;
export module lock_free_queue;
import std;
import std.compat;


NAMESPACE_BEGIN
export template <typename T>
class SPSCQueue {
    size_t capacity_ = 0;
    std::vector<T> queue_{};
    alignas(64)
    std::atomic<size_t> read_{};
    alignas(64)
    std::atomic<size_t> write_{};
public:
    SPSCQueue() = delete;
    explicit SPSCQueue(const size_t capacity) : capacity_(capacity + 1) {
        queue_.resize(capacity_);
    }

    bool push(const T& item) {
        auto pos = write_.load(std::memory_order_relaxed);
        auto next = (pos + 1) % capacity_;

        if (next == read_.load(std::memory_order_acquire))
            return false;

        queue_[pos] = item;
        write_.store(next, std::memory_order_release);
        return true;
    }
    std::optional<T> pop() {
        auto pos = read_.load(std::memory_order_relaxed);
        auto next = (pos + 1) % capacity_;

        if (pos == write_.load(std::memory_order_acquire))
            return std::nullopt;

        std::optional<T> item = std::move(queue_[pos]);
        read_.store(next, std::memory_order_release);
        return item;
    }

    size_t size() const {
        auto head = write_.load(std::memory_order_acquire);
        auto tail = read_.load(std::memory_order_acquire);
        return (head + capacity_ - tail) % capacity_;
    }

    bool empty() const {
        auto head = write_.load(std::memory_order_acquire);
        auto tail = read_.load(std::memory_order_acquire);
        return (head == tail);
    }

    void clear() {
        read_.store(0, std::memory_order_relaxed);
        write_.store(0, std::memory_order_relaxed);
    }
};


export template <typename T>
class MPSCQueue {
public:
    MPSCQueue() = default;

    void push(const T& item) {

    }
    std::optional<T> pop() {

    }
};


export template <typename T>
class SPMCQueue {
public:
    SPMCQueue() = default;
    explicit SPMCQueue(size_t capacity) {

    }

    void push(const T& item) {

    }
    std::optional<T> pop() {

    }
};


export template <typename T>
class MPMCQueue {
public:
    void push(const T& item) {

    }
    std::optional<T> pop() {

    }
};


NAMESPACE_END