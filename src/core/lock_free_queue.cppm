/********************************************************************************
* @Author : hexne
* @Date   : 2026/04/17 18:15:55
********************************************************************************/
module;
export module modforge.lock_free_queue;
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

    struct Node {
        T val_{};
        std::atomic<Node*> next_{};

        Node() = default;
        explicit Node(const T &node) : val_(node) {  }
    };

    alignas(64)
    std::atomic<Node *> read_{};
    alignas(64)
    std::atomic<Node *> write_{};

public:
    MPSCQueue() {
        auto head = new Node();

        read_.store(head, std::memory_order_relaxed);
        write_.store(head, std::memory_order_relaxed);
    }

    void push(const T& item) {
        auto node = new Node(item);

        auto pre = write_.exchange(node, std::memory_order_acq_rel);
        pre->next_.store(node, std::memory_order_release);

    }
    std::optional<T> pop() {
        auto head = read_.load(std::memory_order_relaxed);
        auto next = head->next_.load(std::memory_order_acquire);

        if (next == nullptr)
            return std::nullopt;

        auto ret = std::optional<T>(std::move(next->val_));
        read_.store(next, std::memory_order_relaxed);

        delete head;
        return ret;
    }

    ~MPSCQueue() {
        Node *node = read_.load(std::memory_order_relaxed);

        while (node) {
            auto next = node->next_.load(std::memory_order_relaxed);
            delete node;
            node = next;
        }
    }
};


export template <typename T>
class SPMCQueue {
    struct Cell {
        std::atomic<size_t> seq{};
        T data{};
    };

    size_t capacity_{};
    std::unique_ptr<Cell[]> queue_;
    alignas(64)
    std::atomic<size_t> read_{};
    alignas(64)
    std::atomic<size_t> write_{};

public:
    explicit SPMCQueue(size_t capacity)
        : capacity_(capacity),
          queue_(std::make_unique<Cell[]>(capacity))
    {
        for (size_t i = 0; i < capacity_; ++i)
            queue_[i].seq.store(i, std::memory_order_relaxed);
    }

    bool push(const T& item) {
        size_t pos = write_.load(std::memory_order_relaxed);
        Cell& cell = queue_[pos % capacity_];
        size_t seq = cell.seq.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

        if (diff < 0) return false;

        cell.data = item;
        cell.seq.store(pos + 1, std::memory_order_release);
        write_.store(pos + 1, std::memory_order_relaxed);
        return true;
    }

    bool push(T&& item) {
        size_t pos = write_.load(std::memory_order_relaxed);
        Cell& cell = queue_[pos % capacity_];
        size_t seq = cell.seq.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

        if (diff < 0) return false;

        cell.data = std::move(item);
        cell.seq.store(pos + 1, std::memory_order_release);
        write_.store(pos + 1, std::memory_order_relaxed);
        return true;
    }

    std::optional<T> pop() {
        size_t pos = read_.load(std::memory_order_relaxed);
        while (true) {
            Cell& cell = queue_[pos % capacity_];
            size_t seq = cell.seq.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

            if (diff == 0) {
                if (read_.compare_exchange_weak(pos, pos + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed)) {
                    T ret = std::move(cell.data);
                    cell.seq.store(pos + capacity_, std::memory_order_release);
                    return ret;
                }
            } else if (diff < 0) {
                return std::nullopt;
            } else {
                pos = read_.load(std::memory_order_relaxed);
            }
        }
    }
};


export template <typename T>
class MPMCQueue {
    struct Cell {
        std::atomic<size_t> seq{};
        T data{};
    };

    size_t capacity_{};
    std::unique_ptr<Cell[]> queue_;
    alignas(64)
    std::atomic<size_t> read_{};
    alignas(64)
    std::atomic<size_t> write_{};

public:
    explicit MPMCQueue(size_t capacity)
        : capacity_(capacity),
          queue_(std::make_unique<Cell[]>(capacity))
    {
        for (size_t i = 0; i < capacity_; ++i)
            queue_[i].seq.store(i, std::memory_order_relaxed);
    }

    bool push(const T& item) {
        auto pos = write_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = queue_[pos % capacity_];
            auto seq = cell.seq.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);

            if (diff == 0) {
                if (write_.compare_exchange_weak(
                        pos, pos + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed
                    )) {
                    cell.data = item;
                    cell.seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                return false;
            }
            else {
                pos = write_.load(std::memory_order_relaxed);
            }
        }
    }

    std::optional<T> pop() {
        size_t pos = read_.load(std::memory_order_relaxed);

        while (true) {
            Cell& cell = queue_[pos % capacity_];
            auto seq = cell.seq.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);

            if (diff == 0) {
                if (read_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed
                    )) {
                    T ret = std::move(cell.data);
                    cell.seq.store(pos + capacity_, std::memory_order_release);
                    return ret;
                }
            }
            else if (diff < 0) {
                return std::nullopt;
            }
            else {
                pos = read_.load(std::memory_order_relaxed);
            }
        }
    }
};


NAMESPACE_END