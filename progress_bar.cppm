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

export 
class Progressbar {
    std::string name_;
    int total_{};
    int current_{};
    int console_width = Console::get_width();

    static constexpr int number_width = 5;
    int width = console_width - number_width;

    int name_width = width * 0.20;
    int tab_width = width * 0.05;
    int bar_width = width * 0.75;
public:
    Progressbar(std::string name, int total) : name_(std::move(name)), total_(total) {  }

    Progressbar& operator += (int num) {
        current_ += num;
        return *this;
    }
    std::string get_name() const {
        if (name_.size() < name_width) {
            std::string format = std::string() + "{:" + std::to_string(name_width) + "}";
            return std::vformat(format, std::make_format_args(name_));
        }
        std::string format = std::string() + "{:." + std::to_string(name_width) + "}...";
        return std::vformat(format, std::make_format_args(name_));
    }

    std::string get_bar() const {
        std::string bar(bar_width, ' ');
        bar.front() = '[';
        bar.back() = ']';
        float percentage = current_ * 1.f / total_;
        int cur_bar_width = (bar_width - 2) * percentage ;
        for (int i = 0;i < cur_bar_width; ++i)
            bar[i+1] = '-';
        if (cur_bar_width)
            bar[cur_bar_width] = '>';
        return bar;
    }

    void set_current(int current) {
        current_ = current;
    }

    std::string get_percentage() const {
        float percentage = current_ * 1.f / total_;
        return std::format("{:>4}%", static_cast<int>(percentage * 100));
    }

    std::string get_progress_bar() const {
        auto name = std::async(&Progressbar::get_name, this);
        // tab
        auto tab = std::string(tab_width, ' ');

        // 进度条
        auto bar = std::async(&Progressbar::get_bar, this);

        // 百分比
        auto percentage = std::async(&Progressbar::get_percentage, this);

        return name.get() + tab + bar.get() + percentage.get();
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
    std::vector<Progressbar> bars_;

    void push(Progressbar pb) {
        total_ ++;
        bars_.emplace_back(std::move(pb));
    }

    std::string get_percentage_bar() {
        std::string ret;

        return ret;
    }

};
