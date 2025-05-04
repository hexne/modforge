/*******************************************************************************
 * @Author : hexne
 * @Data   : 2022/05/09 11:29:07
*******************************************************************************/
module;
#include <iostream>
#include <string>
#include "tools.h"
export module ProgressBar;

export
NAMESPACE_BEGIN(nl)


class ProgressBar {
    int total_ = 0;
    int now_ = 0;


public:
    int wide = 100;
    explicit ProgressBar(const int total) : total_(total) {  }
    ProgressBar(const int now, const int total) {
        now_ = now;
        total_ = total - 1 < 0 ? 0 : total - 1;
    }

    [[nodiscard]]
    size_t get_percentage() const {
        return now_ * 100 / total_;
    }
    [[nodiscard]]
    std::string string() const {
        const auto percentage = get_percentage();
        std::string progressbar(wide > 2 ? wide : 2, '-');

        for (int i = 1; i < wide * percentage / 100; ++i) {
            progressbar[i] = '#';
        }
        progressbar[0] = '[';
        progressbar[progressbar.size() - 1] = ']';
        return progressbar;
    }

    ProgressBar& operator ++() {
        this->now_++;
        if (now_ > total_)
            now_ = total_;
        return *this;
    }
    ProgressBar operator ++(int) {
        auto cp = *this;
        now_++;
        if (now_ > total_)
            now_ = total_;
        return cp;
    }
};

std::ostream& operator << (std::ostream& out, const ProgressBar &pb) {
    out << pb.string() << " " << pb.get_percentage() << "%" << "\r";
    out.flush();
    return out;
}



NAMESPACE_END(nl)
