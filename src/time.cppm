/*******************************************************************************
 * @Author : hexne
 * @Data   : 2023/12/03
*******************************************************************************/
module;
#include <string>
#include <chrono>
#include <print>
#include <ranges>
export module modforge.time;
// import std;

template <typename T>
concept ClockType = std::is_same_v<T, std::chrono::nanoseconds>
                ||  std::is_same_v<T, std::chrono::microseconds>
                ||  std::is_same_v<T, std::chrono::milliseconds>
                ||  std::is_same_v<T, std::chrono::seconds>
                ||  std::is_same_v<T, std::chrono::minutes>
                ||  std::is_same_v<T, std::chrono::hours>;

template<typename T>
concept DateType = std::is_same_v<T, std::chrono::days>
                || std::is_same_v<T, std::chrono::months>
                || std::is_same_v<T, std::chrono::years>
                || std::is_same_v<T, std::chrono::weeks>;


template <typename T>
concept TimeType = ClockType<T> || DateType<T>;

export
enum class TimeZone {
    UTC, CST, Local
};
constexpr std::array time_zone_map {
    "UTC",
    "CST"
};

/**
 * @brief 根据参数创建ymd
 * @param y: 指定年份，公元前为负数
 * @param m: 指定月份
 * @param d: 指定日期
 * @return 返回构造的ymd
**/
auto get_ymd(int y, int m, int d) {
    return std::chrono::year_month_day {
        std::chrono::year(y),
        std::chrono::month(m),
        std::chrono::day(d)
    };
}

/**
 * @brief 获取公元初始日期 1/1/1
**/
auto get_begin_ymd() {
    return get_ymd(1, 1, 1);
}

/**
 * @brief 根据参数创建指定hms
 * @param h: 指定hour
 * @param m: 指定minute
 * @param s: 指定second
 * @return 返回构造出的hms
**/
template <typename TimePrecision>
auto get_hms(int h, int m, int s) {
    return std::chrono::hh_mm_ss<TimePrecision> {
        std::chrono::hours {h} +
        std::chrono::minutes {m} +
        std::chrono::seconds {s}
    };
}

/**
 * @brief 获取初始时钟 0:0:0
**/
template <typename TimePrecision>
auto get_begin_hms() {
    return get_hms<TimePrecision>(0, 0, 0);
}

/**
 * @brief 将指定的ymd和hms拼接成time_point
 * @param ymd: 指定的ymd
 * @param hms: 指定的hms
 * @return 返回拼接成的time_point
**/
template <typename TimePrecision>
auto cat_ymd_and_hms(const std::chrono::year_month_day &ymd, const std::chrono::hh_mm_ss<TimePrecision> &hms) {
    return std::chrono::time_point<std::chrono::system_clock, TimePrecision> {
        std::chrono::sys_days(ymd).time_since_epoch() +
            hms.to_duration()
    };
}

/**
 * @brief 仅根据ymd生成time_point
 * @param ymd: 指定的ymd
 * @return 生成的time_point
**/
template <typename TimePrecision>
auto create_time_point_by_ymd(const std::chrono::year_month_day &ymd) {
    return cat_ymd_and_hms(ymd, get_begin_hms<TimePrecision>());
}

/**
 * @brief 仅根据hms生成time_point
 * @param hms: 指定的hms
 * @return 返回生成的time_point
**/
template <typename TimePrecision>
auto create_time_point_by_hms(const std::chrono::hh_mm_ss<TimePrecision> &hms) {
    return cat_ymd_and_hms<TimePrecision>(get_begin_ymd(), hms);
}

/**
 * @brief 获取公元初始时间点 1/1/1 0:0:0
**/
template <typename TimePrecision>
auto get_begin_time_point() {
    return cat_ymd_and_hms(get_begin_ymd(), get_begin_hms<TimePrecision>());
}

/**
 * @brief 获取指定时区
**/
template <TimeZone time_zone>
constexpr auto zone() {
    if constexpr (time_zone == TimeZone::Local) {
        return std::chrono::current_zone();
    }
    else {
        return std::chrono::locate_zone(time_zone_map.at(static_cast<int>(time_zone)));
    }
}

/**
 * @brief 带时区的时间点转为utc时间点
 * @param zone_time: 时区时间
 * @return 转换后的UTC时间点
**/
template <typename TimePrecision>
auto get_utc_time_point_by_zone_time(const std::chrono::zoned_time<TimePrecision> &zone_time) {
    return zone_time.get_sys_time();
}

/**
 * @brief 将时间点转为特定时区的时间
 * @param time_point: 指定时间点
 * @return 转换后的时间点
**/
template <TimeZone time_zone, typename TimePrecision, typename ToPrecision = TimePrecision>
auto get_zoned_time_by_utc_time_point(const std::chrono::time_point<std::chrono::system_clock, TimePrecision> &time_point) {
    return std::chrono::zoned_time { zone<time_zone>(), std::chrono::time_point_cast<ToPrecision>(time_point) };
}

/**
 * @brief 在进行加减运算时，会错误的产生公元0年，此时使用该值进行修正
**/
auto zero_year_duration() {
    return std::chrono::days{366};
}

export
template<TimeZone time_zone = TimeZone::UTC, typename TimePrecision = std::chrono::milliseconds>
class TimeImpl {
    using time_precision = TimePrecision;

    using ymd_t = std::chrono::year_month_day;
    using hms_t = std::chrono::hh_mm_ss<time_precision>;
    using time_point_t = std::chrono::time_point<std::chrono::system_clock, time_precision>;

    std::chrono::zoned_time<time_precision> time_;

public:

    /**
     * @brief 获取指定年份的某个月份有多少天
     * @param year : 指定年份
     * @param month : 指定月份
     * @return 指定年份的某个月份有多少天
    **/
    static constexpr int get_days_in_month(int year, int month) {
        if (month == 2) {
            return (year % 400 == 0) || (year % 100 != 0 && year % 4 == 0) ? 29 : 28;
        }
        int days[] = {0, 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        return days[month];
    }

    /**
     * @brief 从当前时间点构造Time
    **/
    static TimeImpl now() {
        return TimeImpl {std::chrono::system_clock::now()};
    }

    /**
     * @brief 从带有Date的字符串构造Time year-month-day 或 year/month/day
    **/
    static TimeImpl from_date(std::string_view date) {
        std::array<int, 3> nums{};
        int pos{};
        for (auto &&part :  date
                            | std::views::split([](char ch) {
                                 return ch == '-' || ch == '/';
                            })
                            | std::views::transform([](auto && rang) {
                                return std::string(rang.begin(), rang.end());
                            })
                            | std::views::take(3)) {
            nums[pos++] = std::stoi(part);
        }
        return from_date(nums[0], nums[1], nums[2]);
    }

    /**
     * @brief 指定y,m,d 构造Time
     * @param y: 指定的year, 公元前为负数
     * @param m: 指定的month
     * @param d: 指定的day
     * @return 返回构造出的ymd
    **/
    static TimeImpl from_date(int y, int m, int d) {
        return TimeImpl(::get_ymd(y, m, d));
    }
    static TimeImpl from_date(const ymd_t& ymd) {
        return TimeImpl(ymd);
    }

    // 从clock构造Time
    static TimeImpl from_clock(std::string_view clock) {
        std::array<int, 3> nums{};
        int pos{};
        for (auto &&part :  clock
                            | std::views::split([](char ch) {
                                 return ch == '-' || ch == '/';
                            })
                            | std::views::transform([](auto && rang) {
                                return std::string(rang.begin(), rang.end());
                            })
                            | std::views::take(3)) {
            nums[pos++] = std::stoi(part);
        }
        return from_date(nums[0], nums[1], nums[2]);
    }
    static TimeImpl from_clock(int h, int m, int s) {
        return TimeImpl(::get_hms<time_precision>(h, m, s));
    }
    static TimeImpl from_clock(const hms_t& hms) {
        return TimeImpl(hms);
    }

    /**
     * @brief 生成公元初始时间 1/1/1 0:0:0
    **/
    TimeImpl() : time_(get_zoned_time_by_utc_time_point<time_zone>(get_begin_time_point<time_precision>())) {  }

    /**
     * @brief 从ymd构造Time
    **/
    explicit TimeImpl(const ymd_t& ymd) {
        time_ = get_zoned_time_by_utc_time_point<time_zone, time_precision>(create_time_point_by_ymd<time_precision>(ymd));
    }

    /**
     * @brief 从hms构造Time
    **/
    explicit TimeImpl(const hms_t& hms) {  }

    /**
     * @brief 从指定时间点构造Time
     * @param time_point: 指定时间点
    **/
    explicit TimeImpl(const std::chrono::system_clock::time_point &time_point) {
        auto new_time_point = std::chrono::time_point_cast<time_precision>(time_point);
        time_ = get_zoned_time_by_utc_time_point<time_zone, std::chrono::nanoseconds, time_precision>(time_point);
    }

    /**
     * @brief 指定ymd和hms构造time
     * @param ymd: 指定的ymd
     * @param hms: 指定的hms
    **/
    TimeImpl(const ymd_t &ymd, const hms_t &hms) {
        TimeImpl(cat_ymd_and_hms<time_precision>(ymd, hms));
    }

    /**
     * @brief 从指定的日期和时间构造Time
     * @param year: 指定年
     * @param month: 指定月
     * @param day: 指定日期
     * @param hour: 指定小时
     * @param minute: 指定分钟
     * @param second: 指定秒
    **/
    TimeImpl(int year, int month, int day, int hour, int minute, int second) {
        time_ = get_zoned_time_by_utc_time_point<time_zone, time_precision>(cat_ymd_and_hms(
            ::get_ymd(year, month, day),
            ::get_hms<time_precision>(hour, minute, second)
        ));
    }

    /**
     * @brief 从时间间隔构造出 距离 公元 1/1/1 0:0:0 的时间
     * @param duration: 时间间隔
    **/
    TimeImpl(const time_precision &duration) {
        auto time_point = get_begin_time_point<time_precision>();
        time_point += duration;
        time_ = get_zoned_time_by_utc_time_point<time_zone, time_precision>(time_point);
    }

    explicit operator time_precision() const {
        return time_.get_sys_time().time_since_epoch() - get_begin_time_point<time_precision>().time_since_epoch();
    }

    TimeImpl(const TimeImpl& time) noexcept = default;
    TimeImpl(TimeImpl &&time) noexcept = default;

    TimeImpl &operator = (const TimeImpl &time) noexcept = default;
    TimeImpl &operator = (TimeImpl &&time) noexcept = default;

    ~TimeImpl() noexcept = default;

    /**
     * @brief 将时间点移至现在
     * @return 修改时间点后的Time&
    **/
    TimeImpl& to_now() {
        *this = now();
        return *this;
    }

    /**
     * @brief 获得对应的本地时间
     * @return 返回对应的本地时间
    **/
    TimeImpl<TimeZone::Local, TimePrecision> get_local_time() {
        if constexpr (time_zone == TimeZone::Local)
            return *this;
        else
            return TimeImpl<TimeZone::Local, TimePrecision>(time_.get_sys_time());
    }

    template <TimeType T>
    TimeImpl operator + (T count) const {
        TimeImpl ret(*this);
        ret += count;
        return ret;
    }

    template <TimeType T>
    TimeImpl operator - (T count) const {
        TimeImpl ret(*this);
        ret -= count;
        return ret;
    }

    template <TimeType T>
    TimeImpl& operator += (T count) {
        TimeImpl tmp(count);
        *this += tmp;
        return *this;
    }

    template <TimeType T>
    TimeImpl& operator -= (T count) {
        TimeImpl tmp(count);
        *this -= tmp;
        return *this;
    }

    TimeImpl operator + (const TimeImpl &time) {
        TimeImpl ret(*this);
        ret += time;
        return ret;
    }
    TimeImpl operator - (const TimeImpl &time) {
        TimeImpl ret(*this);
        ret -= time;
        return ret;
    }

    TimeImpl& operator += (const TimeImpl &time) {
        auto cur_time_point = time_.get_sys_time();
        auto res = cur_time_point + static_cast<time_precision>(time);

        if (cur_time_point < get_begin_time_point<time_precision>() // 如果向上遇到了公元0年，多加一年进行修正
            && res > cat_ymd_and_hms(::get_ymd(0, 0, 0), ::get_hms<time_precision>(0, 0, 0)))
            res += zero_year_duration();

        time_ = get_zoned_time_by_utc_time_point<time_zone, time_precision>(res);
        return *this;
    }
    TimeImpl& operator -= (const TimeImpl &time) {
        auto sub_res = time_.get_sys_time().time_since_epoch() - time.time_.get_sys_time().time_since_epoch();
        auto time_point = get_begin_time_point<time_precision>() + sub_res;

        if (time_point < get_begin_time_point<time_precision>())
            time_point -= zero_year_duration();     // 1/1/1 向下减会产生公元0年，此处多减一年进行修正

        auto zoned = get_zoned_time_by_utc_time_point<time_zone, time_precision>(time_point);
        time_ = zoned;
        return *this;
    }

    /**
     * @brief 获取当前时间点在该日期中的各个部分，比如小时部分，分钟部分等
     * @return 获取部分的数值
    **/
    template <typename T>
    int get() const requires ( std::is_same_v<T, std::chrono::year>
                            || std::is_same_v<T, std::chrono::month>
                            || std::is_same_v<T, std::chrono::day>
                            || TimeType<T> ) {
        if constexpr (std::is_same_v<T, std::chrono::year> || std::is_same_v<T, std::chrono::years>)
            return static_cast<int>(get_ymd().year().operator int());
        else if constexpr (std::is_same_v<T, std::chrono::month> || std::is_same_v<T, std::chrono::months>)
            return static_cast<int>(get_ymd().month().operator unsigned());
        else if constexpr (std::is_same_v<T, std::chrono::day> || std::is_same_v<T, std::chrono::days>)
            return static_cast<int>(get_ymd().day().operator unsigned());
        else if constexpr (std::is_same_v<T, std::chrono::weeks>)
            return static_cast<int>(std::chrono::weekday(get_hms()).iso_encoding());
        else if constexpr (std::is_same_v<T, std::chrono::hours>)
            return static_cast<int>(get_hms().hours().count());
        else if constexpr (std::is_same_v<T, std::chrono::minutes>)
            return static_cast<int>(get_hms().minutes().count());
        else if constexpr (std::is_same_v<T, std::chrono::seconds>)
            return static_cast<int>(get_hms().seconds().count());
        else if constexpr (std::is_same_v<T, std::chrono::milliseconds>)
            return static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    time_.get_sys_time().time_since_epoch()
                    - std::chrono::floor<std::chrono::seconds>(time_.get_sys_time().time_since_epoch())).count()
                );
        throw std::invalid_argument("error type");
    }

    /**
     * @brief 获取std::chrono::year_month_day
     * @return 返回当前时间对应的ymd
    **/
    ymd_t get_ymd() const {
        return std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(time_.get_local_time()));
    }

    hms_t get_hms() const {
        auto time_point = time_.get_local_time();
        const auto since_midnight = time_point - std::chrono::floor<std::chrono::days>(time_point);
        const auto since_midnight_sec = std::chrono::duration_cast<time_precision>(since_midnight);
        return hms_t(since_midnight_sec);
    }

    std::string get_date_string() const {
        return std::format("{}-{}-{}", get<std::chrono::year>(), get<std::chrono::month>(), get<std::chrono::day>());
    }
    std::string get_clock_string() const {
        return std::format("{}:{}:{}", get<std::chrono::hours>(), get<std::chrono::minutes>(), get<std::chrono::seconds>());
    }
    std::string get_string() const {
        return get_date_string() + " " + get_clock_string();
    }

    /**
     * @brief 统计和基准时间相比过去了多久
    **/
    template <TimeType T>
    long long count() const {
        auto fixup = std::chrono::time_point_cast<T>(std::chrono::system_clock::time_point()).time_since_epoch() - get_begin_time_point<time_precision>().time_since_epoch(); // 时间修正
        auto same_fixup = std::chrono::duration_cast<time_precision>(fixup);
        return std::chrono::duration_cast<T>(time_.get_sys_time().time_since_epoch() + same_fixup).count();
    }

    bool operator == (const TimeImpl &time) const {
        return time_ == time.time_;
    }

    auto operator <=> (const TimeImpl &time) const {
        return time_.get_sys_time() <=> time.time_.get_sys_time();
    }

    friend std::ostream &operator << (std::ostream &out, const TimeImpl &time) {
        out <<time.get_string();
        return out;
    }

};

export using Time = TimeImpl<TimeZone::UTC, std::chrono::milliseconds>;

export using LocalTime = TimeImpl<TimeZone::Local, std::chrono::milliseconds>;

/**
 * @param 't' 完整的time
 * @param 'd' date 时间
 * @param 'c' clock 时间
 * @param '' 为空时保持默认time输出
 * @param 格式化输出样例: ':*^30d'
**/
export template <typename  T>
struct FormatterIMPL {
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

    constexpr auto format(const T& time, auto& context) const {
        std::string time_str {};
        switch (format_type_) {
            case FormatType::None:
            case FormatType::Time:
                time_str = time.get_string();
                break;
            case FormatType::Date:
                time_str = time.get_date_string();
                break;
            case FormatType::Clock:
                time_str = time.get_clock_string();
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

template <>
struct std::formatter<Time> : FormatterIMPL<Time> {  };

template <>
struct std::formatter<LocalTime> : FormatterIMPL<LocalTime> {  };

