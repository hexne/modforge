/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/24 01:36:03
********************************************************************************/

module;
#if defined(_WIN32)
// mingw modules bug workaround：windows.h 内部链在 extern "C" 上下文拉入 c++config.h 与
// import std 冲突，先 include <cstddef> 置位其 guard 可避开（7 个 WIN 头模块同法）
#include <cstddef>
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif
export module modforge.terminal;
import std;

NAMESPACE_BEGIN
export namespace terminal {

    int width();
    int height();

    void clear();

    int cursor_x();
    int cursor_y();

    void cursor_x(int x);
    void cursor_y(int y);

    std::tuple<int, int> cursor();
    void cursor(int x, int y);

    void hide_cursor();
    void show_cursor();

}

#if defined(_WIN32)

// Windows 空实现：仅保证编译/链接；尺寸与光标查询返回 0，操作空转，待接 WinAPI/ANSI
int terminal::width() { return 0; }

int terminal::height() { return 0; }

void terminal::clear() { }

int terminal::cursor_x() { return 0; }

int terminal::cursor_y() { return 0; }

void terminal::cursor_x(int) { }
void terminal::cursor_y(int) { }

std::tuple<int, int> terminal::cursor() { return {0, 0}; }
void terminal::cursor(int, int) { }

void terminal::hide_cursor() { }
void terminal::show_cursor() { }

#elif defined(__linux__) || defined(__unix__)

int terminal::width() {
    winsize size{};

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

    return size.ws_col;
}

int terminal::height() {
    winsize size{};

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

    return size.ws_row;
}

void terminal::clear() {
    std::print("\033[2J\033[H");
    std::cout.flush();
}

int terminal::cursor_x() {
    return std::get<0>(cursor());
}

int terminal::cursor_y() {
    return std::get<1>(cursor());
}

void terminal::cursor_x(const int x) {
    std::print("\033[{}G", x + 1);
    std::cout.flush();
}

void terminal::cursor_y(const int y) {
    std::print("\033[{}d", y + 1);
    std::cout.flush();
}

std::tuple<int, int> terminal::cursor() {
    std::print("\033[6n");
    std::cout.flush();

    char buffer[32]{};
    const auto size = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

    if (size <= 0)
        return {0, 0};

    int x{};
    int y{};

    std::sscanf(
        buffer,
        "\033[%d;%dR",
        &y,
        &x
    );

    return {
        x - 1,
        y - 1
    };
}

void terminal::cursor(const int x, const int y) {
    std::print(
        "\033[{};{}H",
        y + 1,
        x + 1
    );

    std::cout.flush();
}

void terminal::hide_cursor() {
    std::print("\033[?25l");
    std::cout.flush();
}

void terminal::show_cursor() {
    std::print("\033[?25h");
    std::cout.flush();
}

#else

static_assert(false, "Error OS");

#endif

NAMESPACE_END
