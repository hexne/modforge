/********************************************************************************
 * @Author : hexne
 * @Date   : 2025/04/18
********************************************************************************/
export module modforge.timer;
import std;
import modforge.time;
import modforge.id_generator;


NAMESPACE_BEGIN
export class Timer {
    using Interval = std::chrono::milliseconds;
    using Time = UTCTime<Interval>;
    using CallbackFunc = std::function<void()>;

    // 任务
    struct Task {
        int id;
        CallbackFunc callback{};
        Interval interval{};
        Time end;
        bool is_repeat_task{}; // 无限循环
        int repeat_count{}; // 多次循环或者单次

        bool operator < (const Task& task) const {
            return end > task.end;
        }
        bool operator == (const Task& task) const {
            return id == task.id;
        }
    };

    std::mutex mutex_;
    std::priority_queue<Task> tasks_;
    std::jthread thread_;
    std::condition_variable cv_{};

    IDGenerator id_gen_;
    int create_id() {
        return id_gen_.next();
    }



    void run() {
        while (!is_finish()) {
            if (tasks_.empty()) {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this] { return is_finish() || !tasks_.empty(); });
            }
            else {
                Task task;
                {
                    std::unique_lock lock(mutex_);
                    task = tasks_.top();

                    cv_.wait_until(lock, task.end.time_point(), [this] {
                        return is_finish() || tasks_.empty() || tasks_.top().end <= Time::now();
                    });
                }
                if (is_finish())
                    break;
                if (tasks_.empty())
                    continue;
                task = tasks_.top();
                task.callback();
                {
                    std::lock_guard lock(mutex_);
                    tasks_.pop();
                    if (task.is_repeat_task || --task.repeat_count > 0) {
                        task.end += task.interval;
                        tasks_.push(task);
                    }
                }
            }
        }
    }

public:
    Timer() : thread_(std::jthread(&Timer::run,this)) {  }

    [[nodiscard]]
    bool is_finish() const {
        return thread_.get_stop_token().stop_requested();
    }

    int add_task(CallbackFunc callback, Interval interval, bool is_repeat = false) {
        std::lock_guard lock(mutex_);
        auto id = create_id();
        tasks_.push(Task {
            .id = id,
            .callback = std::move(callback),
            .interval = interval,
            .end = Time::now() + interval,
            .is_repeat_task = is_repeat
        });
        cv_.notify_one();
        return id;
    }
    int add_repeat_task(CallbackFunc callback, Interval interval) {
        return add_task(std::move(callback), interval, true);
    }
    int add_task(CallbackFunc callback, Interval interval, int repeat_count) {
        std::lock_guard lock(mutex_);
        auto id = create_id();
        tasks_.push(Task {
            .id = id,
            .callback = std::move(callback),
            .interval = interval,
            .end = Time::now() + interval,
            .repeat_count = repeat_count
        });
        cv_.notify_one();
        return id;
    }

    void remove(int id) {
        std::lock_guard lock(mutex_);
        std::priority_queue<Task> new_tasks;
        while (!tasks_.empty()) {
            auto task = tasks_.top();
            tasks_.pop();
            if (task.id != id)
                new_tasks.push(task);
        }
        tasks_ = new_tasks;
    }

    int task_count() {
        std::lock_guard lock(mutex_);
        return tasks_.size();
    }

    ~Timer() {
        cv_.notify_all();
        thread_.request_stop();
        thread_.join();
    }


};


export class CoroutineTimer {
    struct Task {
        Time end;
        std::coroutine_handle<> handle;
        bool operator<(const Task &other) const { return end > other.end; }
    };
    std::priority_queue<Task> tasks_;

    void add_task(const Time &deadline, std::coroutine_handle<> handle) {
        tasks_.push(Task{deadline, handle});
    }

public:
    class TimerAwaiter {
        CoroutineTimer *timer_;
        Time deadline_;
    public:
        TimerAwaiter(CoroutineTimer *timer, const Time &deadline)
            : timer_(timer), deadline_(deadline) {}

        bool await_ready() const { return deadline_ <= Time::now(); }
        void await_suspend(std::coroutine_handle<> handle) {
            timer_->add_task(deadline_, handle);
        }
        void await_resume() const {}
    };

    TimerAwaiter sleep_for(std::chrono::steady_clock::duration d) {
        return TimerAwaiter{this, Time::now() + d};
    }
    TimerAwaiter sleep_until(const Time &time_point) {
        return TimerAwaiter{this, time_point};
    }

    std::optional<Time> resume() {
        const Time now = Time::now();
        while (!tasks_.empty() && tasks_.top().end <= now) {
            auto handle = tasks_.top().handle;
            tasks_.pop();
            handle.resume();
        }
        if (tasks_.empty()) return std::nullopt;
        return tasks_.top().end;
    }

    bool empty() const { return tasks_.empty(); }
};

export template <typename T>
struct TimerTaskPromise;

export template <typename T>
class TimerTask {
public:
    using promise_type = TimerTaskPromise<T>;

    explicit TimerTask(std::coroutine_handle<promise_type> h) : handle_(h) {}
    TimerTask(TimerTask &&other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    TimerTask(const TimerTask &) = delete;
    TimerTask &operator=(TimerTask &&other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    ~TimerTask() { if (handle_) handle_.destroy(); }

    bool done() const { return handle_.done(); }
    T result() const { return handle_.promise().value; }
    std::coroutine_handle<> raw_handle() const { return handle_; }

private:
    std::coroutine_handle<promise_type> handle_;
};

template <typename T>
struct TimerTaskPromise {
    T value{};

    TimerTask<T> get_return_object() {
        return TimerTask<T>{std::coroutine_handle<TimerTaskPromise>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(T v) { value = std::move(v); }
    void unhandled_exception() { std::terminate(); }
};


NAMESPACE_END
