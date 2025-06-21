/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/
module;
#include <future>
#include <string>
#include <iostream>
#include <ranges>
#include <vector>
export module modforge.progress_bar;

import modforge.console;
import modforge.time;

std::string get_name(const std::string &name, size_t width) {
    if (name.size() < width) {
        std::string format = std::string() + "{:" + std::to_string(width) + "}";
        return std::vformat(format, std::make_format_args(name));
    }
    std::string format = std::string() + "{:." + std::to_string(width) + "}...";
    return std::vformat(format, std::make_format_args(name));
}

std::string get_remaining_time(const Time &begin_time, size_t current, size_t total, size_t width) {
    std::string ret;
    auto cur_time = Time::now();
    auto use_time = (cur_time - begin_time).count<std::chrono::seconds>();

    float percentage = current * 1.f / total;
    size_t total_time = use_time / percentage;
    auto remaining_time = total_time - use_time;
    if (percentage >= 1.f)
        remaining_time = 0;
    auto end_time = cur_time;
    end_time += std::chrono::seconds(remaining_time);

    std::string format = "{:^" + std::to_string(width) + "}";
    auto arg = (end_time - cur_time).get_clock_string();
    return std::vformat(format, std::make_format_args(arg));
}

std::string get_bar(size_t current, size_t total, size_t width) {
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


std::string get_percentage(size_t current, size_t total, size_t width) {
    float percentage = current * 1.f / total;
    std::string format = "{:>" + std::to_string(width) + "}%";
    int arg = percentage * 100;
    return std::vformat(format, std::make_format_args(arg));
}


export
class Progressbar {
    Time begin_time_;
    std::string name_;
    int total_{};
    int current_{};
    int console_width = Console::get_width();

    static constexpr int percentage_width = 5;
    static constexpr int time_width = 10;
    int width = console_width - time_width - percentage_width;

    int name_width = width * 0.20;
    int tab_width = width * 0.05;
    int bar_width = width * 0.75;
public:
    Progressbar(std::string name, int total) : name_(std::move(name)), total_(total) {  }

    Progressbar& operator += (int num) {
        current_ += num;
        return *this;
    }


    void set_current(int current) {
        current_ = current;
    }
    size_t get_current() const {
        return current_;
    }


    std::string get_progress_bar() const {
        auto name = std::async(&get_name, name_, name_width);
        // tab
        auto tab = std::string(tab_width, ' ');

        // 剩余时间
        auto time = std::async(&get_remaining_time, begin_time_, current_, total_, time_width);

        // 进度条
        auto bar = std::async(&get_bar, current_, total_, bar_width);

        // 百分比
        auto percentage = std::async(&get_percentage, current_, total_, percentage_width);

        return name.get() + tab + time.get() + bar.get() + percentage.get();
    }

    void print() {
        std::cout << get_progress_bar() << '\r';
        std::cout.flush();
    }

};
export
class Progress {
    int console_width = Console::get_width();
    int number_width{};
    int tab_width{};
    int time_width = 10;
    int bar_width{};
    int percentage_width{};
    int total_{}, current_{};
    std::vector<std::pair<std::string, int>> bars_;
    Progressbar cur_bar_;

    Time begin_time_;


    size_t get_max_number_width() const {
        std::vector<size_t> number_widths(bars_.size());
        for (int i = 0;i < number_widths.size(); ++i)
            number_widths[i] = std::to_string(bars_[i].second).size();
        return *std::max_element(number_widths.begin(), number_widths.end());
    }

public:

    void push(const std::string& name, int total) {
        total_ ++;
        bars_.emplace_back(name, total);
    }
    Progressbar &cur_bar() {
        return cur_bar_;
    }
    Progress& operator += (int num) {
        current_ += num;
        return *this;
    }

    std::string get_progress_bar() {
        auto first_line = std::async(&Progressbar::get_progress_bar, &cur_bar_);

        // 数字
        auto get_number = std::async([this](size_t current, size_t total, size_t width) {
            std::string format = "({:>" + std::to_string(width) + "}/{})";
            auto args = std::make_format_args(current, total);
            return std::vformat(format, args);
        }, current_, total_, get_max_number_width());

        // 缩进
        auto tab = std::string(tab_width, ' ');

        // 剩余时间
        size_t cur{};
        for (int i = 0;i < current_; ++i)
            cur += bars_[i].second;
        cur += cur_bar().get_current();
        size_t total{};
        for (const auto &bar : bars_)
            total += bar.second;
        auto time = std::async(&get_remaining_time, begin_time_, cur, total, time_width);

        // 进度条
        auto bar = std::async(&get_bar, cur, total_, bar_width);

        // 百分比
        auto percentage = std::async(&get_percentage, current_, total_, percentage_width);

        return first_line.get() + '\n' + get_number.get() + tab + time.get() + bar.get() + percentage.get();
    }


    void print() {
        std::cout << get_progress_bar() << '\r';
        Console::cursor_up();
    }

};


/*

Progress progress;
progress.push("归一化"，1000);
progress.push("训练中"，10000);

while (归一化中)
    progress.cur_bar() += 1;
    progress.print();

progress += 1;
while(训练中)
    progress.cur_bar() += 1;
    progress.print();















 */