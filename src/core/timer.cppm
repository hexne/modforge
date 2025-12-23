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
        bool is_repeat_task{};
        Time end;

        bool operator < (const Task& task) const {
            return end >                              task.end;
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
                    if (task.is_repeat_task) {
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

    void add_task(CallbackFunc func, Interval interval, const bool is_repeat_task = false) {
        std::lock_guard lock(mutex_);
        tasks_.push(
            { create_id(), std::move(func) , interval, is_repeat_task, Time::now() + interval }
        );
    }

    void add_repeat_task(CallbackFunc func, Interval interval) {
        add_task(std::move(func), std::move(interval), true);
    }

    ~Timer() = default;


};