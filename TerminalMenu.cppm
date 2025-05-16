/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/11/24 11:48
*******************************************************************************/

module;
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
export module modforge.terminal_menu;


class TerminalMenu {
    struct Options {
        std::string title;
        std::function<void()> action;
    };
    std::vector<Options> menu_;
public:
    friend std::ostream& operator<<(std::ostream& os, const TerminalMenu& menu) {
        for (const auto& [title, action] : menu.menu_) {
            std::cout << title << '\n';
        }
        std::endl(std::cout);
        return os;
    }

    TerminalMenu() { }
    void add_menu(std::string name, std::function<void()> func) {
        menu_.emplace_back(Options {name, func});
    }
    void clear() {
#ifdef __WIN32
        system("cls");
#elif __linux__
        system("clear");
#endif
    }

    void start() {
        while (true) {
            clear();
            std::cout << *this;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};