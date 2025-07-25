
/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/

module;
#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <sys/ioctl.h>  // ioctl() 和 TIOCGWINSZ
#include <unistd.h>     // STDOUT_FILENO
#endif
export module modforge.console;

import std;

export 
namespace Console {
	void hind_cursor();
	void show_cursor();

    void cursor_up(int = 1);
    void cursor_down(int = 1);

	void set_color();
	void clear_color();

    size_t get_width();
    size_t get_height();

};

#ifdef _WIN32

void Console::hind_cursor() {
	CONSOLE_CURSOR_INFO cursor_info { 1, 0};
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}
void Console::show_cursor() {
    CONSOLE_CURSOR_INFO cursor_info{ 1, 1 };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void Console::cursor_up(int lines) {
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    GetConsoleScreenBufferInfo(h_console, &console_info);

    COORD new_pos = console_info.dwCursorPosition;
    new_pos.Y -= (SHORT)lines;
    if (new_pos.Y < 0) new_pos.Y = 0;

    SetConsoleCursorPosition(h_console, new_pos);
}

void Console::cursor_down(int lines) {
    HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    GetConsoleScreenBufferInfo(h_console, &console_info);

    COORD new_pos = console_info.dwCursorPosition;
    new_pos.Y += (SHORT)lines;

    SetConsoleCursorPosition(h_console, new_pos);
}

size_t Console::get_width() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}
size_t Console::get_height() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#elif __linux__

void Console::hind_cursor() {
    std::cout << "\033[?25l";
}
void Console::show_cursor() {
    std::cout << "\033[?25h";
}

void Console::cursor_up(int num) {
    std::cout << std::format("\033[{}A", num);
}
void Console::cursor_down(int num) {
    std::cout << std::format("\033[{}B", num);
}
size_t Console::get_width() {
    const char* columns = std::getenv("COLUMNS");
    if (columns) {
        return std::stoul(columns);
    }

    // 回退到 ioctl
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
        return size.ws_col;
    }

    return 80; // 默认值
}

size_t Console::get_height() {
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    return size.ws_row;
}
#endif
