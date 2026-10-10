/********************************************************************************
* @Author : hexne
* @Date   : 2026/03/10 20:09:52
********************************************************************************/
module;
export module modforge.thread_pool;
import modforge.lock_free_queue;
import std;
import std.compat;
import modforge.os;


void bind_thread_to_core(std::thread &t, int core_id) {
#if defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc = pthread_setaffinity_np(t.native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
#elif defined(_WIN32)
    DWORD_PTR mask = 1ULL << core_id;
    HANDLE hThread = (HANDLE)t.native_handle();
    DWORD_PTR result = SetThreadAffinityMask(hThread, mask);
    if (result == 0) {
        std::cerr << "Error calling SetThreadAffinityMask\n";
    }
#else
    (void)t; (void)core_id;
    std::cerr << "CPU affinity not supported on this platform.\n";
#endif
}

NAMESPACE_BEGIN
export class ThreadPool {
    using Task = std::function<void()>;

    size_t thread_count_;
    // 多线程并发 submit 会同时递增，必须是原子量（普通 size_t 自增是数据竞争）
    std::atomic<size_t> idx_{};
    std::vector<std::thread> workers_;
    std::vector<std::unique_ptr<SPMCQueue<Task>>> local_queues_;
    std::atomic_bool stop_{false};

    void loop(size_t idx) {
        while (true) {
            std::optional<Task> task;

            for (size_t j = 0; j < thread_count_; ++j) {
                task = local_queues_[(idx + j) % thread_count_]->pop();
                if (task) break;
            }

            if (task) {
                (*task)();
            } else if (stop_.load(std::memory_order_acquire)) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    }

public:
    explicit ThreadPool(
        size_t n = std::max(1u, std::thread::hardware_concurrency()),
        size_t capacity = 1024)
        : thread_count_(n)
    {
        local_queues_.reserve(n);
        for (size_t i = 0; i < n; ++i)
            local_queues_.emplace_back(std::make_unique<SPMCQueue<Task>>(capacity));

        workers_.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this, i]{ loop(i); });
            bind_thread_to_core(workers_[i], i);
        }
    }

    template <typename Fun, typename... Args>
    auto submit(Fun&& func, Args&&... args) {
        using Ret = std::invoke_result_t<Fun, Args...>;
        auto task = std::make_shared<std::packaged_task<Ret()>>(
            [func = std::forward<Fun>(func),
             ...args = std::forward<Args>(args)]() mutable {
                return func(std::forward<Args>(args)...);
            });
        auto future = task->get_future();

        size_t idx = idx_++ % thread_count_;
        while (!local_queues_[idx]->push([task]{ (*task)(); })) {
            std::this_thread::yield();
        }

        return future;
    }

    ~ThreadPool() {
        stop_.store(true, std::memory_order_release);
        for (auto& w : workers_)
            if (w.joinable()) w.join();
    }
};
NAMESPACE_END