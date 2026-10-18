/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/
export module modforge.progress;
import std;
import modforge.time;
import modforge.terminal;

NAMESPACE_BEGIN
std::string remaining_time(int cur, int total, const Time &begin, int width) {
    auto now = Time::now();
    if (total < 0 || cur < 0 || now < begin)
        throw std::invalid_argument("invalid args");

    // cur == 0 时完成比例为 0，外推会除出 inf（转整数是 UB）：ETA 不可知
    // total == 0 时 percentage 为 inf，use_time / inf = 0 自然得到 0:0:0，无需特判
    std::string string = "--:--:--";
    if (cur != 0) {
        const auto use_time = (now - begin).count<std::chrono::seconds>();
        const double percentage = cur * 1. / total;

        // 线性外推：已用时间 / 完成比例 = 总时长；总时长 - 已用 = 剩余
        const auto total_time = static_cast<long long>(use_time / percentage);
        const auto remaining = total_time > use_time ? total_time - use_time : 0;

        Time ret;
        ret += std::chrono::seconds{remaining};

        // count<> 取到的是总量：小时不取模（剩余超过 24h 不回绕），分/秒取模
        string = std::format("{:2}:{:2}:{:2}", ret.count<std::chrono::hours>(),
                             ret.count<std::chrono::minutes>() % 60,
                             ret.count<std::chrono::seconds>() % 60);
    }
    return std::format("{:^{}}", string, width);
}

template <std::size_t N>
std::string percentage(int cur, int total,int width) {
    if (cur < 0 || total < 0)
        throw std::invalid_argument("percentage must be non-negative");
    // total == 0 为 sub-KB 瞬间完成，与 bar 的满格渲染保持一致
    const double value = total == 0 ? 100. : static_cast<double>(cur) / total * 100;
    auto string = std::format("{:.{}f}%", value, N);
    return std::format("{:{}}", string, width);
}

template <std::array done = { '-', '>' }, char todo = ' '>
std::string bar(int cur, int total, int width) {
    constexpr auto n = done.size();

    // 宽度需放得下 1 个填充字符 + 完整 head（n == 1 时 head 为空，即 width >= 2）
    if (width < static_cast<int>(n) + 1)
        throw std::invalid_argument("width too small for pattern");
    if (total < 0 || cur < 0)
        throw std::invalid_argument("total and cur must be non-negative");

    constexpr char fill = done[0];
    constexpr auto head = std::string_view{done.data(), n}.substr(1);

    const int inner = width - 2;
    // KB 计数下 sub-KB 文件 total 为 0：视为瞬间完成，按满格渲染
    const int pos = total == 0 ? inner : std::min(
        static_cast<int>((static_cast<long long>(inner) * cur + total / 2) / total), inner);

    // head 骑在已推进区域前沿：进度不足画截断的箭头，顶满时完整收尾
    const int drawn = std::min(pos, static_cast<int>(head.size()));
    std::string s{'['};
    s.append(pos - drawn, fill);
    s.append(head.data(), drawn);
    s.append(inner - pos, todo);
    s += ']';
    return s;
}

// 名位段：短名居中补齐，长名按字节截断加 "..."。
// UTF-8 字节数 >= 显示列数，按字节截断只会多切不会少切，帧宽不会超
std::string name(const std::string &name, int width) {
    if (width <= 0)
        return {};
    if (static_cast<int>(name.size()) <= width)
        return std::format("{:^{}}", name, width);
    return std::format("{:.{}}...", name, std::max(width - 3, 0));
}


std::string count(int cur, int total) {
    auto cur_str = std::to_string(cur);
    auto total_str = std::to_string(total);
    int number_width = std::max(cur_str.size(), total_str.size());

    return std::format("[{:>{}}/{:{}}]", cur_str, number_width, total_str, number_width);
}

// 前置声明：ProgressBar 的友元声明要引用 Progress<N, percentage_width>，
// 默认模板实参在此给出，后面定义处不能再重复
export template<std::size_t N = 0, std::array<double, 4> percentage_width = { 0.15, 0.10, 0.65, 0.10 }>
class Progress;

export template<std::size_t N = 0, std::array<double, 4> percentage_width = { 0.15, 0.10, 0.65, 0.10 }>
class ProgressBar {
    friend class Progress<N, percentage_width>;
    std::string name_{};
    int cur_{};
    int total_{};
    Time begin_ = Time::now();
    int width_ = terminal::width();
public:
    ProgressBar() {
        width_ -= 3;
        if (width_ <= 0)
            width_ = 80;
    }
    ProgressBar(std::string name, int total) : ProgressBar() {
        total_ = total;
        name_ = std::move(name);
        if (total < 0)
            throw std::invalid_argument("total must be greater than 0");
    }

    ProgressBar& operator+=(int value) {
        cur_ += value;
        return *this;
    }
    ProgressBar& operator ++() {
        this->operator+=(1);
        return *this;
    }

    std::string string() const {
        int name_width = width_ * percentage_width[0];
        int time_width = width_ * percentage_width[1];
        int percent_width = width_ * percentage_width[3];
        int bar_width = width_ - name_width - time_width - percent_width;
        return std::format("{} {} {} {}",
            name(name_, name_width),
            remaining_time(cur_, total_, begin_, time_width),
            bar(cur_, total_, bar_width),
            percentage<N>(cur_, total_, percent_width));
    }

    void print(std::ostream &out = std::cout) const {
        std::print(out, "\r{}", string());
    }

};

template<std::size_t N, std::array<double, 4> percentage_width>
class Progress {
    std::vector<ProgressBar<>> bars_;
    ProgressBar<N, percentage_width> total_bar_;
    int cur_{};
    int last_cur_{};
    int total_{};

    // cur_ == bars_.size() 即 done 哨兵态：允许循环末尾统一 ++ 到达该态，
    // 但该态下使用型操作（cur_progress_bar/string/print）一律抛异常
    void check_not_done() const {
        if (cur_ >= static_cast<int>(bars_.size()))
            throw std::out_of_range("progress already done");
    }
public:
    explicit Progress(const std::initializer_list<ProgressBar<>> &bars) : bars_(bars), total_(bars.size()) { }

    Progress& append(const ProgressBar<> &bar) {
        bars_.push_back(bar);
        total_ ++;
        return *this;
    }

    ProgressBar<>& cur_progress_bar() {
        check_not_done();
        return bars_[cur_];
    }

    Progress& operator++() {
        check_not_done();
        ++ cur_;
        return *this;
    }
    std::string string() const {
        check_not_done();
        const auto &cur_bar = bars_[cur_];

        // 细粒度总进度：已完成的阶段各计 100，当前阶段按自身 cur/total 折算成 0~100，
        // 总量纲 = 阶段数 × 100。bar/percentage/remaining_time 只认 cur/total 比例，直接复用
        // cur_bar.total_ == 0（sub-KB 瞬间完成）折算 100，与 bar/percentage 的满格特判一致
        const int cur_part = cur_bar.total_ == 0 ? 100
            : static_cast<int>(static_cast<long long>(cur_bar.cur_) * 100 / cur_bar.total_);
        const int fine_cur = cur_ * 100 + cur_part;
        const int fine_total = total_ * 100;

        // 宽度分配与 ProgressBar::string() 一致，基准取 total_bar_.width_
        const int w = total_bar_.width_;
        const int name_width = w * percentage_width[0];
        const int time_width = w * percentage_width[1];
        const int percent_width = w * percentage_width[3];
        const int bar_width = w - name_width - time_width - percent_width;

        return std::format("{}\n{}", cur_bar.string(),
            std::format("{} {} {} {}",
                // [x/y] 现场算：缓存 name_ 依赖 ++ 刷新时机，最终帧会停在构造时的 [1/2]
                name(count(cur_ + 1, total_), name_width),
                remaining_time(fine_cur, fine_total, total_bar_.begin_, time_width),
                bar(fine_cur, fine_total, bar_width),
                percentage<N>(fine_cur, fine_total, percent_width)));
    }

    void print(std::ostream &out = std::cout) {
        check_not_done();
        const auto &cur_bar = bars_[cur_];

        if (cur_ != last_cur_) {
            last_cur_ = cur_;
            std::println(out, "");
        }
        std::print(out, "{}", string());
        modforge::terminal::cursor_up(1);
        std::print(out, "\r");

        // 最后一个阶段也已推进到头：光标此刻停在两行帧的首行行首，两个换行
        // 把它推到最终画面下方，保全最后两行不被后续输出覆盖。
        // cur_ 是 0-based 索引，最后阶段是 total_-1；>= 防超涨误判，total_==0（瞬间完成）恒真
        if (cur_ == total_ - 1 && cur_bar.cur_ >= cur_bar.total_) {
            std::println(out, "");
            std::println(out, "");
        }
    }



};


NAMESPACE_END

