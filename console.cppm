
/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/

module;
#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <iostream>
#include <format>
#endif
export module modforge.console;

export 
struct Console {
	static void hind_cursor();
	static void show_cursor();

    static void cursor_up(int = 1);
    static void cursor_down(int = 1);

	static void set_color();
	static void clear_color();

};

module : private;
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


#endif
