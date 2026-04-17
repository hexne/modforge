/********************************************************************************
* @Author : hexne
* @Date   : 2026/04/17 18:15:55
********************************************************************************/

module;
export module unlocked_queue;
import std;
import std.compat;


NAMESPACE_BEGIN
export template <typename T>
class SPSCQueue {
public:
    SPSCQueue() = default;
    explicit SPSCQueue(size_t capacity) {

    }

    bool push(const T& item) {

    }
    std::optional<T> pop() {

    }

    size_t size() const {

    }
    bool   empty() const {

    }
    size_t capacity() const {

    }

    void clear() {

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