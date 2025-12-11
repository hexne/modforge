/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 21:27:07
********************************************************************************/
module;
export module modforge.progress_bar;

import std;
import modforge.range;
import modforge.console;
import modforge.time;

std::string get_name(const std::string &name, std::size_t width) {
    if (name.size() <= width) {
        std::string format = std::string() + "{:" + std::to_string(width) + "}";
        return std::vformat(format, std::make_format_args(name));
    }
    std::string format = std::string() + "{:." + std::to_string(width - 3) + "}...";
    return std::vformat(format, std::make_format_args(name));
}

std::string get_remaining_time(const Time& begin_time, std::size_t current, std::size_t total, std::size_t width) {
    std::string format = "{:^" + std::to_string(width) + "}";
    return std::vformat(format, std::make_format_args("-"));
    // std::string ret;
    // auto cur_time = Time::now();
    // auto use_time = (cur_time - begin_time).count<std::chrono::seconds>();
    //
    // float percentage = current * 1.f / total;
    // std::size_t total_time = use_time / percentage;
    // auto remaining_time = total_time - use_time;
    // if (percentage >= 1.f)
    //     remaining_time = 0;
    // auto end_time = cur_time;
    // end_time += std::chrono::seconds(remaining_time);
    //
    // std::string format = "{:^" + std::to_string(width) + "}";
    // auto arg = (end_time - cur_time).get_clock_string();
    // return std::vformat(format, std::make_format_args(arg));
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

export
template <typename T>
class ProgressBar {
    Time begin_time_;
    std::string name_;
    int total_{};
    int current_{};
    int console_width = Console::width();
    bool destruct_print_endl_ = true;

    static constexpr int percentage_width = 5;
    static constexpr int time_width = 10;
    int width = console_width - time_width - percentage_width;

    int name_width = width * 0.20;
    int tab_width = width * 0.05;
    int bar_width = width * 0.75;


public:
    ProgressBar(std::string name, int total, bool print_endl = true)
        : name_(std::move(name)), total_(total), destruct_print_endl_(print_endl) {  }

    ProgressBar(std::string name, const Range<T> &range, bool print_endl = true)
        : ProgressBar(name, range.distance(), print_endl) {  }

    ProgressBar& operator += (int num) {
        current_ += num;
        return *this;
    }

    void current(int current) {
        current_ = current;
    }
    [[nodiscard]] std::size_t current() const {
        return current_;
    }
    void destruct_print_endl(bool flag) {
        destruct_print_endl_ = flag;
    }


    std::string get_progress_bar() const {
        auto name = std::async(get_name, name_, name_width);
        // tab
        auto tab = std::string(tab_width, ' ');

        // 剩余时间
        auto time = std::async(get_remaining_time, begin_time_, current_, total_, time_width);

        // 进度条
        auto bar = std::async(get_bar, current_, total_, bar_width);

        // 百分比
        auto percentage = std::async(get_percentage, current_, total_, percentage_width);

        return name.get() + tab + time.get() + bar.get() + percentage.get();
    }

    void print() const {
        std::print("{}\r", get_progress_bar());
    }
    friend std::ostream& operator << (std::ostream& out, const ProgressBar& bar) {
        out << bar.get_progress_bar() << '\r';
        out.flush();
        return out;
    }

    ~ProgressBar() {
        if (destruct_print_endl_)
            endl(std::cout);
    }

};

ProgressBar(std::string, int) -> ProgressBar<int>;
ProgressBar(std::string, int, bool) -> ProgressBar<int>;

// export template <typename T>
// ProgressBar(std::string, Range<T>) -> ProgressBar<T>;
// export template <typename T>
// ProgressBar(std::string, Range<T>, bool) -> ProgressBar<T>;