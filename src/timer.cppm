/********************************************************************************
 * @Author : hexne
 * @Date   : 2025/04/18
********************************************************************************/
export module modforge.timer;
import std;
import modforge.time;


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

    int id_{};
    int create_id() {
        return id_ ++;
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
NAMESPACE_END
