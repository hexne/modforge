/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/
module;
#include <string>
export module modforge.progress_bar;


import modforge.console;

export
class Progress {
    // 终端的前35% 用于输出内容，中间5%空格和..., 50%用于进度条，10%用于百分比显示
    int console_width = Console::get_width();
    int total_{}, current_{};

    // 项目数量, 每个项目应该输出 当前进行的项目字符串， 例如：安装pip中... [#########-----] 50%
    explicit Progress(size_t total) : total_(total) {

    }
};

export 
class Progressbar {
    std::string name;
    int total_{};
    int console_width = Console::get_width();
public:
    Progressbar(const std::string &name, int total) : total_(total) {

    }

};
