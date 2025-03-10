/*******************************************************************************
 * Author : yongheng
 * Data   : 2023/12/03
*******************************************************************************/
module;
#include <iostream>
#include <algorithm>
#include <chrono>
#include <functional>
#include <mutex>
#include <ranges>
#include <set>
#include <thread>
#include "tools.h"
export module Time;

time_t GetNowTimeCount() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

template<class T>
concept CountType = std::is_same_v<T, std::chrono::nanoseconds>
    || std::is_same_v<T, std::chrono::microseconds>
    || std::is_same_v<T, std::chrono::milliseconds>
    || std::is_same_v<T, std::chrono::seconds>
    || std::is_same_v<T, std::chrono::minutes>
    || std::is_same_v<T, std::chrono::hours>
    || std::is_same_v<T, std::chrono::days>
    || std::is_same_v<T, std::chrono::weeks>;

export
NAMESPACE_BEGIN(nl)

class Time {
    std::chrono::zoned_time<std::chrono::system_clock::duration> time_;
public:
    Time() {
        time_ = std::chrono::zoned_time(std::chrono::current_zone()->name(),std::chrono::system_clock::now());
    }
    Time(const Time& time) : time_(time.time_) {  }
    explicit Time(const std::chrono::zoned_time<std::chrono::system_clock::duration> &time) : time_(time) {  }

    explicit Time(const std::chrono::time_point<std::chrono::system_clock> time) : time_(time) {  }

    /**
     * @param time_str format need "year-month-day hour:minute:second"
    **/
    explicit Time(const std::string &time_str) {
        std::vector<std::string> time_node;

        std::string tmp;
        for (const char ch : time_str) {
            if (ch == ' ' || ch == ':' || ch == '-') {
                time_node.push_back(tmp);
                tmp.clear();
                continue;
            }
            if (ch < '0' || ch > '9')
                throw std::invalid_argument("invalid time string");
            tmp += ch;
        }
        time_node.push_back(tmp);

        if (time_node.size() != 6)
            throw std::invalid_argument("invalid time string");

        auto ymd = std::chrono::year_month_day(
                std::chrono::year(std::stoi(time_node[0])),
                std::chrono::month(std::stoi(time_node[1])),
                std::chrono::day(std::stoi(time_node[2]))
            );
        auto hms = std::chrono::hh_mm_ss<std::chrono::seconds>(
                std::chrono::seconds{
                    std::stoi(time_node[3]) * 3600 + std::stoi(time_node[4]) * 60 + std::stoi(time_node[5])
                }
            );
        time_ = typename std::chrono::local_time<std::chrono::seconds>::time_point(std::chrono::local_days(ymd).time_since_epoch() + hms.to_duration());
    }

    Time operator - (const Time& time) const {
        return Time(std::chrono::time_point<std::chrono::system_clock>(time_.get_local_time() - time.time_.get_local_time()));
    }

    template<CountType T = std::chrono::milliseconds>
    size_t count() {
        return std::chrono::duration_cast<T>(time_.get_local_time().time_since_epoch()).count();
    }

    Time& to_now() {
        time_ = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        return *this;
    }

    [[nodiscard]]
    std::string to_string() const {
        std::stringstream ss;

        const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(time_.get_local_time())};
        ss << ymd << ' ';
        const std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(time_.get_local_time()) - std::chrono::floor<std::chrono::days>(time_.get_local_time())};
        ss << hms;

        return ss.str();
    }
    [[nodiscard]]
    std::string to_date() const {
        std::stringstream ss;
        const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(time_.get_local_time())};
        ss << ymd;
        return ss.str();
    }
    [[nodiscard]]
    std::string to_clock() const {
        std::stringstream ss;
        const std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(time_.get_local_time()) - std::chrono::floor<std::chrono::days>(time_.get_local_time())};
        ss << hms;
        return ss.str();
    }

    friend std::ostream& operator << (std::ostream& out,const Time &time) {
        out << time.to_string();
        return out;
    }

};


class Timer {

    using CallbackFunc = std::function<void()>;

    struct TimerTask {
        CallbackFunc callback_func;
        time_t end_time;
        bool is_repeat_task = false;
        time_t interval_duration;

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

    void add_task(CallbackFunc func, const time_t time, const bool is_repeat_task = false) {
        std::lock_guard lock(tasks_mutex_);
        tasks_.insert(
            { std::move(func) , GetNowTimeCount() + time, is_repeat_task, time }
        );
    }

    void add_repeat_task(CallbackFunc func, const time_t time) {
        add_task(std::move(func), std::move(time), true);
    }

    [[nodiscard]]
    size_t task_count(bool only_count_once_task = false) {
        std::lock_guard lock(tasks_mutex_);
        if (only_count_once_task) {
            return std::ranges::count_if(tasks_, [](const TimerTask& task) {
                return !task.is_repeat_task;
                });
        }
        return tasks_.size();
    }

};

class CountTime {
    Time time_begin_;
    Time time_end_;
public:

    void start_count() {
        time_begin_ = Time();
    }
    void end_count() {
        time_end_ = Time();
    }

    template <typename T = std::chrono::milliseconds>
    int count() const {
        return (time_end_ - time_begin_).count<T>();
    }

};


NAMESPACE_END(nl)

// use demo, std::format("{:*^30c}",time), 支持 c，d, t, 和 空'{:*^30}'
template <>
struct std::formatter<nl::Time> {
    constexpr auto parse(auto& context) {
        auto d_it = std::find_if( context.begin(), context.end(),[](auto ch) {
            return ch == 'd';
        });
        if (d_it != context.end()) {
            format_type_ = FormatType::Date;
            format_ = "{:" + std::string(context.begin(), d_it) + "}";
            return d_it + 1;
        }

        auto c_it = std::find_if( context.begin(), context.end(),[](auto ch) {
            return ch == 'c';
        });
        if (c_it != context.end()) {
            format_type_ = FormatType::Clock;
            format_ = "{:" + std::string(context.begin(), c_it) + "}";
            return c_it + 1;
        }
        auto t_it = std::find_if( context.begin(), context.end(),[](auto ch) {
            return ch == 't';
        });
        if (t_it != context.end()) {
            format_type_ = FormatType::Time;
            format_ = "{:" + std::string(context.begin(), t_it) + "}";
            return t_it + 1;
        }

        auto end = std::find_if( context.begin(), context.end(),[](auto ch) {
            return ch == '}';
        });
        format_type_ = FormatType::None;
        format_ = "{:" + std::string(context.begin(), end) + "}";
        return end;
    }

    constexpr auto format(const nl::Time& time, auto& context) const {
        std::string time_str {};
        switch (format_type_) {
            case FormatType::None:
            case FormatType::Time:
                time_str = time.to_string();
                break;
            case FormatType::Date:
                time_str = time.to_date();
                break;
            case FormatType::Clock:
                time_str = time.to_clock();
                break;
            default:
                break;
        }
        return std::vformat_to(context.out(), format_, std::make_format_args(time_str));
    }
private:
    enum class FormatType {
        None, Time, Date, Clock
    } format_type_ {};
    std::string format_ {};
};