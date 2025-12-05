/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/
export module modforge.progress_bar;

import std;
import modforge.time;
import modforge.console;

template <typename T>
struct is_container : std::false_type { };

template <typename T>
    requires requires (T t) { t.begin(); t.end(); }
struct is_container<T> : std::true_type { };

export template <typename T>
class Range;

template <typename T>
    requires std::is_integral_v<T>
class Range<T>{
public:
    class iterator {
        T val_{};
    public:
        iterator() = default;
        explicit iterator(T val) : val_(val) {  }
        iterator& operator++() {
            ++ val_;
            return *this;
        }
        bool operator != (const iterator &other) {
            return val_ != other.val_;
        }
        T& operator *() {
            return val_;
        }
        T operator - (const iterator& other) const {
            return val_ - other.val_;
        }
    };
    Range(int begin, int end) : begin_(iterator(begin)), end_(iterator(end)) {  }
    iterator begin() {
        return begin_;
    }
    iterator end() {
        return end_;
    }
private:
    iterator begin_, end_;
};

template <typename T>
    requires std::is_pointer_v<T>
class Range<T> {
public:
    class iterator {
        T point_{};
    public:
        iterator() = default;
        explicit iterator(T point) : point_(point) {  }
        iterator& operator++() {
            ++ point_;
            return *this;
        }
        bool operator != (const iterator &other) {
            return point_ != other.point_;
        }
        std::remove_pointer_t<T>& operator *() {
            return *point_;
        }
        int operator - (const iterator& other) {
            return point_ - other.point_;
        }
    };
    Range(T begin_pointer, T end_pointer) : begin_(begin_pointer), end_(end_pointer) {  }
    iterator begin() {
        return begin_;
    }
    iterator end() {
        return end_;
    }
private:
    iterator begin_, end_;
};

template <typename T>
    requires is_container<T>::value
class Range<T> {
public:
    class iterator {
        typename T::iterator iterator_{};
    public:
        iterator() = default;
        explicit iterator(decltype(iterator_) iterator) : iterator_(iterator) { }

        iterator& operator++() {
            ++ iterator_;
            return *this;
        }
        bool operator != (const iterator &other) const {
            return iterator_ != other.iterator_;
        }
        int operator - (const iterator& other) const {
            return iterator_ - other.iterator_;
        }
        auto& operator *() {
            return *iterator_;
        }
    };

    explicit Range(T &container) : begin_(container.begin()), end_(container.end()) {  }
    auto begin() {
        return begin_;
    }
    auto end() {
        return end_;
    }
private:
    iterator begin_, end_;
};

template <typename T>
Range(T,  T) -> Range<T>;

template <typename T>
Range(T) -> Range<T>;



std::string get_name(const std::string &name, std::size_t width) {
    if (name.size() <= width) {
        std::string format = std::string() + "{:" + std::to_string(width) + "}";
        return std::vformat(format, std::make_format_args(name));
    }
    std::string format = std::string() + "{:." + std::to_string(width - 3) + "}...";
    return std::vformat(format, std::make_format_args(name));
}

std::string get_remaining_time(const Time &begin_time, std::size_t current, std::size_t total, std::size_t width) {
    std::string ret;
    auto cur_time = Time::now();
    auto use_time = (cur_time - begin_time).count<std::chrono::seconds>();

    float percentage = current * 1.f / total;
    std::size_t total_time = use_time / percentage;
    auto remaining_time = total_time - use_time;
    if (percentage >= 1.f)
        remaining_time = 0;
    auto end_time = cur_time;
    end_time += std::chrono::seconds(remaining_time);

    std::string format = "{:^" + std::to_string(width) + "}";
    auto arg = (end_time - cur_time).get_clock_string();
    return std::vformat(format, std::make_format_args(arg));
}

std::string get_bar(std::size_t current, std::size_t total, std::size_t width) {
    std::string bar(width, ' ');
    bar.front() = '[';
    bar.back() = ']';
    float percentage = current * 1.f / total;
    int cur_bar_width = (width - 2) * percentage ;
    for (int i = 0;i < cur_bar_width; ++i)
        bar[i+1] = '-';
    if (cur_bar_width)
        bar[cur_bar_width] = '>';
    return bar;
}


std::string get_percentage(std::size_t current, std::size_t total, std::size_t width) {
    float percentage = current * 1.f / total;
    std::string format = "{:>" + std::to_string(width) + "}%";
    int arg = percentage * 100;
    return std::vformat(format, std::make_format_args(arg));
}


export template <typename T>
class ProgressBar {
public:
    class iterator {
        typename Range<T>::iterator it_{};
        ProgressBar *pb_{};
    public:
        iterator() = default;
        explicit iterator(typename Range<T>::iterator it, ProgressBar *pb = nullptr) : it_(it), pb_(pb) {  }

        iterator & operator++() {
            std::print("{}\r", pb_->prograss_bar_string());
            ++ it_;
            return *this;
        }
        bool operator != (const iterator &other) {
            return it_ != other.it_;
        }
        auto& operator *() {
            return *it_;
        }
        int operator - (const iterator &other) {
            return it_ - other.it_;
        }

    };

public:
    ProgressBar(int total, const std::string &title = "") {
        Range<T> range(0, total);
        begin_ = iterator(range.begin(), this);
        end_ = iterator(range.end(), this);
        title_ = title;
        total_ = end_ - begin_;
        cur_ = 0;
    }
    ProgressBar(T begin_pointer, T end_pointer, const std::string &title = "") {
        Range<T> range(begin_pointer, end_pointer);
        begin_ = iterator(range.begin(), this);
        end_ = iterator(range.end(), this);
        title_ = title;
        total_ = end_ - begin_;
        cur_ = 0;
    }
    ProgressBar(T &container, const std::string &title = "") {
        Range<T> range(container);
        begin_ = iterator(range.begin(), this);
        end_ = iterator(range.end(), this);
        title_ = title;
        total_ = end_ - begin_;
        cur_ = 0;
    }
    ProgressBar(Range<T> range, const std::string &title = "") {
        begin_ = iterator(range.begin(), this);
        end_ = iterator(range.end(), this);
        title_ = title;
        total_ = end_ - begin_;
        cur_ = 0;
    }

    auto begin() {
        begin_time_ = Time::now();
        Console::hind_cursor();
        return begin_;
    }
    auto end() {
        return end_;
    }

    std::string prograss_bar_string() {
        ++ cur_;
        int console_width = Console::get_width();
        static constexpr int percentage_width = 5;
        static constexpr int time_width = 10;
        int width = console_width - time_width - percentage_width;
        int name_width = width * 0.20;
        int tab_width = width * 0.05;
        int bar_width = width * 0.75;

        auto name = std::async(get_name, title_, name_width);
        // tab
        auto tab = std::string(tab_width, ' ');

        // 剩余时间
        auto time = std::async(get_remaining_time, begin_time_, cur_, total_, time_width);

        // 进度条
        auto bar = std::async(get_bar, cur_, total_, bar_width);

        // 百分比
        auto percentage = std::async(get_percentage, cur_, total_, percentage_width);

        return name.get() + tab + time.get() + bar.get() + percentage.get();
    }
    ~ProgressBar() {
        Console::show_cursor();
        std::println("");
    }
private:
    iterator begin_, end_;
    std::string title_{};
    int total_{}, cur_{};
    Time begin_time_;

};
template <typename T>
    requires std::is_integral_v<T>
ProgressBar(T, const std::string &title = "") -> ProgressBar<int>;

template <typename T>
ProgressBar(T, T, const std::string &title = "") -> ProgressBar<T>;

template <typename T>
    requires is_container<T>::value
ProgressBar(T&, const std::string &title = "") -> ProgressBar<T>;
