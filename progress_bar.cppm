/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/
module;
#include <future>
#include <string>
#include <iostream>
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

    static constexpr int number_width = 5;
    static constexpr int time_width = 10;
    int width = console_width - time_width - number_width;

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


    std::string get_progress_bar() const {
        auto name = std::async(&get_name, name_, name_width);
        // tab
        auto tab = std::string(tab_width, ' ');

        auto time = std::async(&get_remaining_time, begin_time_, current_, total_, time_width);

        // 进度条
        auto bar = std::async(&get_bar, current_, total_, bar_width);

        // 百分比
        auto percentage = std::async(&get_percentage, current_, total_, number_width);

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
    int total_{}, current_{};
    std::vector<std::pair<std::string, int>> bars_;

    void push(const std::string& name, int total) {
        total_ ++;
        bars_.emplace_back(name, total);
    }
    Progress& operator += (int num) {
        current_ += num;

        return *this;
    }

    std::string get_percentage_bar() {
        std::string ret;

        return ret;
    }


    void print() {
        // 输出当前progress bar
        Progressbar pb(bars_[current_].first, bars_[current_].second);
        std::cout << pb.get_progress_bar() << std::endl;

        // 输出总进度

    }

};
