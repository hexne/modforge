/********************************************************************************
 * @Author : hexne
 * @Date   : 2025/04/18
********************************************************************************/
export module modforge.timer;
import std;
import modforge.time;


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
        int repeat_count{}; // 多次循环或者单词

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

    int id_{};
    int create_id() {
        return id_ ++;
    }



    void run() {
        while (!is_finish()) {
            {
                std::lock_guard lock(mutex_);
                // 首先不能为空, 为空直接结束遍历
                while (!tasks_.empty()) {
                    auto task = tasks_.top();
                    // 优先队列，如果当前不需要执行后面就不需要执行了
                    if (task.end > Time::now()) {
                        break;
                    }
                    tasks_.pop();
                    task.callback();
                    if (task.is_repeat_task or -- task.repeat_count > 0) {
                        task.end += task.interval;
                        tasks_.push(task);
                    }
                }

            }
            std::this_thread::sleep_for(Interval {1});
        }
    }



public:
    Timer() : thread_(std::jthread(&Timer::run,this)) {  }

    [[nodiscard]]
    bool is_finish() const {
        return thread_.get_stop_token().stop_requested();
    }

    int add_task(CallbackFunc callback, Interval interval, bool is_repeat = false) {
        auto id = create_id();
        tasks_.push(Task {
            .id = id,
            .callback = std::move(callback),
            .interval = interval,
            .is_repeat_task = is_repeat
        });
        return id;
    }
    int add_repeat_task(CallbackFunc callback, Interval interval) {
        return add_task(std::move(callback), interval, true);
    }
    int add_task(CallbackFunc callback, Interval interval, int repeat_count) {
        auto id = create_id();
        tasks_.push(Task {
            .id = id,
            .callback = std::move(callback),
            .interval = interval,
            .repeat_count = repeat_count
        });

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

    ~Timer() = default;


};