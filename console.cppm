
/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/

module;
#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <iostream>
#endif
export module modforge.console;

export 
struct Console {
	static void hind_cursor();
	static void show_cursor();

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
    std::cout << "\033[?25lm";
}
void Console::show_cursor() {
    std::cout << "\033[?25hm";
}



#endif
