/********************************************************************************
* @Author : hexne
* @Date   : 2026/03/10 20:09:52
********************************************************************************/
module;
export module modforge.thread_pool;
import std;

NAMESPACE_BEGIN
export class ThreadPool {
    std::vector<std::jthread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_{};

    void loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this]{ return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty())
                    return;

                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }


public:
    explicit ThreadPool(const unsigned int n = std::max(1u, std::thread::hardware_concurrency())) {
        for (int i = 0; i < n; ++i)
            threads_.emplace_back([this] {
                loop();
            });

    }
    ~ThreadPool() {
        {
            std::unique_lock lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)  {
        using Ret = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<Ret()>>(
            [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return f(std::forward<Args>(args) ...);
            }
        );
        auto future = task->get_future();

        {
            std::lock_guard lk(mutex_);
            if (stop_)
                throw std::runtime_error("ThreadPool is stopped");
            tasks_.emplace([task] {
                (*task)();
            });
        }
        cv_.notify_one();
        return future;
    }

};

NAMESPACE_END