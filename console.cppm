
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
