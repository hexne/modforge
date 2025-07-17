/********************************************************************************
 * @Author : hexne
 * @Date   : 2025/04/18 15:19:24
********************************************************************************/

export module modforge.timer;
import std;

std::time_t GetNowTimeCount() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}


export class Timer {

    using CallbackFunc = std::function<void()>;

    struct TimerTask {
        CallbackFunc callback_func;
        std::time_t end_time;
        bool is_repeat_task = false;
        std::time_t interval_duration;

        bool operator < (const TimerTask& right) const {
            if (end_time < right.end_time)
                return true;
            return false;
        }
    };

    void run_timer_task(std::set<TimerTask>::const_iterator& it) {
        if (it->is_repeat_task == true) {
            auto task = *it;
            it = tasks_.erase(it);
            task.callback_func();
            task.end_time += task.interval_duration;
            tasks_.insert(task);
        }
        else {
            it->callback_func();
            it = tasks_.erase(it);
        }

    }

    void run() {
        while (!is_finish()) {
            const auto cur_time = GetNowTimeCount();
            for (auto it = tasks_.begin(); it != tasks_.end(); ) {
                if (it->end_time < cur_time) {
                    std::lock_guard lock(tasks_mutex_);
                    run_timer_task(it);
                }
                else
                    ++it;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
        }

    }

    std::jthread thread_;
    std::mutex tasks_mutex_;
    std::multiset<TimerTask> tasks_;

public:
    Timer() : thread_(std::jthread(&Timer::run,this)) {  }

    [[nodiscard]]
    bool is_finish() const {
        return thread_.get_stop_token().stop_requested();
    }

    void add_task(CallbackFunc func, const std::time_t time, const bool is_repeat_task = false) {
        std::lock_guard lock(tasks_mutex_);
        tasks_.insert(
            { std::move(func) , GetNowTimeCount() + time, is_repeat_task, time }
        );
    }

    void add_repeat_task(CallbackFunc func, const std::time_t time) {
        add_task(std::move(func), std::move(time), true);
    }

    [[nodiscard]]
    std::size_t task_count(bool only_count_once_task = false) {
        std::lock_guard lock(tasks_mutex_);
        if (only_count_once_task) {
            return std::ranges::count_if(tasks_, [](const TimerTask& task) {
                return !task.is_repeat_task;
                });
        }
        return tasks_.size();
    }

};