/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/24 01:34:02
********************************************************************************/
module;
export module modforge.table;
import modforge.terminal;
import std;

NAMESPACE_BEGIN

export enum class TextPos {
    left, center, right
};

export struct Text {
    std::string text;
    TextPos pos = TextPos::left;
};

export class Table {
    int terminal_w_ = terminal::width();
    int terminal_h_ = terminal::height();
    int row_{}, col_{};
    std::vector<Text> data_{};
    std::mdspan<Text, std::dextents<std::size_t, 2>> table_;
    std::vector<double> ratio_;

    std::vector<int> get_length() const {
        int width = terminal_w_ - col_ - 1;
        if (width < 0)
            width = 0;

        if (ratio_.empty()) {
            // 平分
            const int length = width / col_;
            return std::vector(col_, length);
        }

        double count{};
        for (auto ratio : ratio_)
            count += ratio;

        if (count > 1)
            throw std::invalid_argument(
                "The ratio sum must not be greater than 1"
            );

        std::vector<int> ret;
        for (int i = 0; i < ratio_.size(); ++i)
            ret.push_back(width * ratio_[i]);

        return ret;
    }

    std::string get_bar() const {
        std::string ret = "+";
        auto lengths = get_length();

        for (int i = 0; i < lengths.size(); ++i) {
            ret += std::string(lengths[i], '-') + "+";
        }

        return ret;
    }

    std::string get_format_text(const Text& text, int length) const {
        std::string fmt;

        switch (text.pos) {
        case TextPos::left:
            fmt = "|{:<{}}";
            break;
        case TextPos::center:
            fmt = "|{:^{}}";
            break;
        case TextPos::right:
            fmt = "|{:>{}}";
            break;
        }

        return std::vformat(
            fmt,
            std::make_format_args(text.text, length)
        );
    }

public:
    Table(const int row, const int col)
        : row_(row), col_(col)
    {
        data_.resize(row_ * col_);
        table_ = std::mdspan(data_.data(), row_, col_);
    }

    void set_title(std::initializer_list<std::string> titles) {
        if (titles.size() != col_)
            throw std::invalid_argument(
                "The title count must equal the column count"
            );

        std::vector vec(titles);

        for (int j = 0; j < vec.size(); ++j)
            table_[0, j].text = vec[j];
    }

    void set_title(int x, int y, const std::string& title) {
        table_[x, y].text = title;
    }

    void set_ratio(std::initializer_list<double> ratio) {
        if (ratio.size() != col_)
            throw std::invalid_argument(
                "The ratio count must equal the column count"
            );

        ratio_ = ratio;
    }

    void set_row(int row, std::initializer_list<std::string> texts) {
        if (texts.size() != col_)
            throw std::invalid_argument(
                "The text count must equal the column count"
            );

        std::vector vec(texts);

        for (int j = 0; j < col_; ++j)
            table_[row, j].text = vec[j];
    }

    void show(bool show_bar = false) {
        auto lengths = get_length();

        std::println("{}", get_bar());

        for (int i = 0; i < table_.extent(0); ++i) {
            for (int j = 0; j < table_.extent(1); ++j) {
                std::print("{}",get_format_text(table_[i, j], lengths[j]));
            }
            std::println("|");
            if (i == 0 || i == table_.extent(0) - 1 || show_bar)
                std::println("{}", get_bar());
        }
    }

    void set_text_pos(int x, int y, TextPos pos) {
        table_[x, y].pos = pos;
    }

    void set_text(int x, int y, const std::string& text) {
        table_[x, y].text = text;
    }

    Text& operator[](const int x, const int y) {
        return table_[x, y];
    }
    int row() const {
        return row_;
    }
    int col() const {
        return col_;
    }
};

NAMESPACE_END