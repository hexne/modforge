/*******************************************************************************
 * @Author : hexne
 * @Data   : 2023/12/03
*******************************************************************************/

module;
#include <iostream>
#include <algorithm>
#include <chrono>
#include <functional>
#include <mutex>
#include <ranges>
#include <thread>
#include "tools.h"
export module Time;

template<class T>
concept CountType = std::is_same_v<T, std::chrono::nanoseconds>
    || std::is_same_v<T, std::chrono::microseconds>
    || std::is_same_v<T, std::chrono::milliseconds>
    || std::is_same_v<T, std::chrono::seconds>
    || std::is_same_v<T, std::chrono::minutes>
    || std::is_same_v<T, std::chrono::hours>
    || std::is_same_v<T, std::chrono::days>
    || std::is_same_v<T, std::chrono::months>
    || std::is_same_v<T, std::chrono::years>
    || std::is_same_v<T, std::chrono::weeks>;

export
NAMESPACE_BEGIN(nl)

class Time {
    std::chrono::zoned_time<std::chrono::system_clock::duration> time_;
public:
    static Time now() {
        return Time(std::chrono::system_clock::now());
    }
    static int count_month_day(int year, int month) {
        if (month == 2) {
            return (year % 400 == 0) || (year % 100 != 0 && year % 4 == 0) ? 29 : 28;
        }
        constexpr int days[] = {0, 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        return days[month];
    }
    Time() {
        time_ = std::chrono::zoned_time(std::chrono::current_zone()->name(),std::chrono::system_clock::now());
    }
    Time(const Time& time) : time_(time.time_) {  }

    explicit Time(const std::chrono::zoned_time<std::chrono::system_clock::duration> &time) : time_(time) {  }

    explicit Time(const std::chrono::time_point<std::chrono::system_clock> time) : time_(time) {  }

    /**
     * @param time format need "year/month/day hour:minute:second" or "year/month/day"
    **/
    explicit Time(const std::string &time) {
        std::vector<std::string> time_node;
        std::string tmp;
        for (const char ch : time) {
            if (ch == ' ' || ch == ':' || ch == '-' || ch == '/') {
                time_node.push_back(tmp);
                tmp.clear();
                continue;
            }
            if (ch < '0' || ch > '9')
                throw std::invalid_argument("invalid time string");
            tmp += ch;
        }
        time_node.push_back(tmp);
        std::chrono::year_month_day ymd{};
        std::chrono::hh_mm_ss<std::chrono::seconds> hms{};

        if (time_node.size() >= 3) {
            ymd = std::chrono::year_month_day(
                    std::chrono::year(std::stoi(time_node[0])),
                    std::chrono::month(std::stoi(time_node[1])),
                    std::chrono::day(std::stoi(time_node[2]))
                );
        }
        if (time_node.size() == 6) {
            hms = std::chrono::hh_mm_ss<std::chrono::seconds>(
                    std::chrono::seconds{
                        std::stoi(time_node[3]) * 3600 + std::stoi(time_node[4]) * 60 + std::stoi(time_node[5])
                    }
                );
        }
        else if (time_node.size() != 3) {
            throw std::invalid_argument("invalid time string");
        }
        time_ = typename std::chrono::local_time<std::chrono::seconds>::time_point(std::chrono::local_days(ymd).time_since_epoch() + hms.to_duration());
    }

    template<CountType T = std::chrono::seconds>
    Time& operator += (T op_time) {
        auto op_num = std::chrono::duration_cast<T>(op_time);
        auto local_time = time_.get_local_time();
        local_time += op_num;
        time_ = std::chrono::zoned_time(time_.get_time_zone(), local_time);
        return *this;
    }

    template<CountType T = std::chrono::seconds>
    Time& operator -= (T op_time) {
        auto op_num = std::chrono::duration_cast<T>(op_time);
        auto local_time = time_.get_local_time();
        local_time -= op_num;
        time_ = std::chrono::zoned_time(time_.get_time_zone(), local_time);
        return *this;
    }
    Time operator - (const Time &other) const {
        Time ret(*this);
        ret -= other;
        return ret;
    }

    // please use ``Time + (const std::chrono::)`` 系列
    Time operator + (const Time &other) = delete;
    Time& operator += (const Time &other) = delete;

    Time& operator-=(const Time &other) {
        time_ = std::chrono::zoned_time{
            time_.get_time_zone(),
            time_.get_sys_time() - other.time_.get_sys_time().time_since_epoch()
        };
        return *this;
    }

    template<CountType T = std::chrono::milliseconds>
    size_t count() const {
        return std::chrono::duration_cast<T>(time_.get_local_time().time_since_epoch()).count();
    }

    size_t get_year() const {
        return std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(time_.get_local_time())).year().operator int();
    }
    size_t get_month() const {
        return std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(time_.get_local_time())).month().operator unsigned();
    }
    size_t get_day() const {
        return std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(time_.get_local_time())).day().operator unsigned();
    }
    size_t get_hour() const {
        return count<std::chrono::hours>() % 24;
    }
    size_t get_minute() const {
        return count<std::chrono::minutes>() % 60;
    }
    size_t get_second() const {
        return count<std::chrono::seconds>() % 60;
    }
    size_t get_week() const {
        return std::chrono::weekday(std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(time_.get_local_time()))).iso_encoding();
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
    std::string to_date_string() const {
        std::stringstream ss;
        const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(time_.get_local_time())};
        ss << ymd;
        return ss.str();
    }
    [[nodiscard]]
    std::string to_clock_string() const {
        std::stringstream ss;
        const std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(time_.get_local_time()) - std::chrono::floor<std::chrono::days>(time_.get_local_time())};
        ss << hms;
        return ss.str();
    }

    friend std::ostream& operator << (std::ostream& out,const Time &time) {
        out << time.to_string();
        return out;
    }


    bool operator == (const Time &time) const {
        return count<std::chrono::seconds>() == time.count<std::chrono::seconds>();
    }
    auto operator <=> (const Time &time) const {
        return std::strong_order(count<std::chrono::seconds>(), time.count<std::chrono::seconds>());
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
                time_str = time.to_date_string();
                break;
            case FormatType::Clock:
                time_str = time.to_clock_string();
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