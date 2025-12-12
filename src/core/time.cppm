/********************************************************************************
* @Author : hexne
* @Date   : 2023/12/03 20:13:56
********************************************************************************/

module;
export module modforge.time;

import std;

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
    utc, cst, local
};
constexpr std::array time_zone_map {
    "UTC", "Asia/Shanghai"
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
 * @brief 获取指定时区
**/
template <TimeZone time_zone>
constexpr auto zone() {
    if constexpr (time_zone == TimeZone::local) {
        return std::chrono::current_zone();
    }
    else {
        return std::chrono::locate_zone(time_zone_map.at(static_cast<int>(time_zone)));
    }
}

/**
 * @brief 将时间点转为特定时区的时间
 * @param time_point: 指定时间点
 * @return 转换后的时间点
**/
export
template <TimeZone time_zone, typename TimePrecision, typename ToPrecision = TimePrecision>
auto get_zoned_time_by_utc_time_point(const std::chrono::time_point<std::chrono::system_clock, TimePrecision> &time_point) {
    return std::chrono::zoned_time { zone<time_zone>(), std::chrono::time_point_cast<ToPrecision>(time_point) };
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

template <TimeZone time_zone, typename TimePrecision>
auto create_zone_time(int year, int month, int day, int hour = 0, int min = 0, int sec = 0) {
    auto ymd = get_ymd(year, month, day);
    auto hms = get_hms<TimePrecision>(hour, min, sec);
    return std::chrono::zoned_time<TimePrecision>(zone<time_zone>(), std::chrono::local_days{ymd} + hms.to_duration());
}

template <TimeZone time_zone, typename TimePrecision>
auto get_begin_zone_time() {
    return create_zone_time<time_zone, TimePrecision>(1, 1, 1, 0, 0, 0);
}

std::vector<int> split_string(std::string string) {
    for (auto &ch : string) {
        if (ch == '/' or ch == '-' or ch == ':')
            ch = ' ';
    }
    std::vector<int> ret;
    for (auto&& str : std::views::split(string, ' ')) {
        ret.push_back(std::stoi(std::string(str.begin(), str.end())));
    }
    return ret;
}

template<TimeZone time_zone = TimeZone::utc, typename TimePrecision = std::chrono::milliseconds>
class TimeImpl {
    using ymd_t = std::chrono::year_month_day;
    using hms_t = std::chrono::hh_mm_ss<TimePrecision>;
    using time_point_t = std::chrono::time_point<std::chrono::system_clock, TimePrecision>;
    std::chrono::zoned_time<TimePrecision> time_;
public:
    static constexpr int get_days_in_month(int year, int month) {
        if (month == 2) {
            return (year % 400 == 0) || (year % 100 != 0 && year % 4 == 0) ? 29 : 28;
        }
        int days[] = {0, 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        return days[month];
    }


    static TimeImpl from_date(int year, int month, int hour) {
        return TimeImpl(year, month, hour, 0, 0, 0);
    }
    static TimeImpl from_date(const std::string& date) {
        auto date_vec = split_string(date);
        if (date_vec.size() != 3)
            throw std::invalid_argument(std::format("arg have error format {}", date));
        return from_date(date_vec[0], date_vec[1], date_vec[2]);
    }

    static TimeImpl from_clock(int hour, int min, int sec) {
        return TimeImpl(1, 1, 1, hour, min, sec);
    }
    static TimeImpl from_clock(const std::string& clock) {
        auto clock_vec = split_string(clock);
        if (clock_vec.size() != 3)
            throw std::invalid_argument(std::format("arg have error format {}", clock));
        return from_clock(clock_vec[0], clock_vec[1], clock_vec[2]);
    }

    // 构造函数
    TimeImpl(const TimeImpl& time) noexcept = default;
    TimeImpl(TimeImpl &&time) noexcept = default;

    TimeImpl &operator = (const TimeImpl &time) noexcept = default;
    TimeImpl &operator = (TimeImpl &&time) noexcept = default;

    ~TimeImpl() noexcept = default;

    // 空的就是1/1/1 0:0:0
    TimeImpl() {
        time_ = create_zone_time<time_zone, TimePrecision>(1, 1, 1, 0, 0, 0);
    }

    explicit TimeImpl(const std::chrono::system_clock::time_point& time_point) {
        time_ = get_zoned_time_by_utc_time_point<time_zone, TimePrecision>(std::chrono::time_point_cast<TimePrecision>(time_point));
    }

    explicit TimeImpl(const std::string& time) {
        auto time_vec = split_string(time);
        if (time_vec.size() != 6)
            throw std::invalid_argument(std::format("arg have error format {}", time));
        time_ = create_zone_time<time_zone, TimePrecision>(time_vec[0], time_vec[1], time_vec[2], time_vec[3], time_vec[4], time_vec[5]);
    }

    TimeImpl(int year, int month, int day, int hour, int min, int sec) {
        time_ = create_zone_time<time_zone, TimePrecision>(year, month, day, hour, min, sec);
    }

    explicit TimeImpl(const TimePrecision& duration) {
        auto tp = get_utc_time_point_by_zone_time(get_begin_zone_time<time_zone, TimePrecision>()) + duration;
        time_ = get_zoned_time_by_utc_time_point<time_zone, TimePrecision>(tp);
    }


    static TimeImpl now() {
        return TimeImpl(std::chrono::system_clock::now());
    }

    TimeImpl& to_now() {
        *this = now();
        return *this;
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
        const auto since_midnight_sec = std::chrono::duration_cast<TimePrecision>(since_midnight);
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
     * @brief 统计和基准时间相比过去了多久
    **/
    template <TimeType T>
    long long count() const {
        auto fixup = std::chrono::time_point_cast<T>(std::chrono::system_clock::time_point()).time_since_epoch() - get_begin_zone_time<time_zone, TimePrecision>().get_sys_time().time_since_epoch(); // 时间修正
        auto same_fixup = std::chrono::duration_cast<TimePrecision>(fixup);
        return std::chrono::duration_cast<T>(time_.get_sys_time().time_since_epoch() + same_fixup).count();
    }

    template <TimeType T>
    TimeImpl& operator += (T count) {
        auto tmp = TimeImpl(count);
        *this += tmp;
        return *this;
    }

    template <TimeType T>
    TimeImpl& operator -= (T count) {
        auto tmp = TimeImpl(count);
        *this -= tmp;
        return *this;
    }

    TimeImpl& operator += (const TimeImpl &time) {
        auto tp = get_utc_time_point_by_zone_time(time_);
        auto time_duration = static_cast<TimePrecision>(time);
        tp += time_duration;
        time_ = get_zoned_time_by_utc_time_point<time_zone, TimePrecision>(tp);
        return *this;
    }
    TimeImpl& operator -= (const TimeImpl &time) {
        auto tp = get_utc_time_point_by_zone_time(time_);
        auto time_duration = static_cast<TimePrecision>(time);
        tp -= time_duration;
        time_ = get_zoned_time_by_utc_time_point<time_zone, TimePrecision>(tp);
        return *this;
    }

    template <TimeType T>
    TimeImpl operator + (T count) {
        TimeImpl ret(*this);
        ret += TimeImpl(count);
        return ret;
    }

    template <TimeType T>
    TimeImpl operator - (T count) {
        TimeImpl ret(*this);
        ret -= TimeImpl(count);
        return ret;
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

    explicit operator TimePrecision() const {
        return time_.get_sys_time().time_since_epoch() - get_begin_zone_time<time_zone, TimePrecision>().get_sys_time().time_since_epoch();
    }

    bool operator==(const TimeImpl& other) const {
        return time_ == other.time_;
    }
    auto operator <=> (const TimeImpl& other) const {
        return time_ <=> other.time_;
    }

    friend std::ostream& operator<< (std::ostream& out, const TimeImpl& time) {
        return out << time.time_;
    }

};


export using Time = TimeImpl<TimeZone::utc, std::chrono::seconds>;

export using LocalTime = TimeImpl<TimeZone::local, std::chrono::seconds>;

export template <typename TimePrecision = std::chrono::microseconds>
using UTCTime = TimeImpl<TimeZone::utc, TimePrecision>;

export template <typename TimePrecision = std::chrono::microseconds>
using CSTTime = TimeImpl<TimeZone::cst, TimePrecision>;

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