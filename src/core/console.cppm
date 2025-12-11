/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/28 14:27:03
********************************************************************************/

module;
#include <sys/ioctl.h>  // ioctl() 和 TIOCGWINSZ
#include <unistd.h>     // STDOUT_FILENO
export module modforge.console;

import std;

export
namespace Console {
	void hind_cursor() {
	    std::print("\033[?25l");
	}
	void show_cursor() {
	    std::print("\033[?25h");
	}

    void cursor_up(int num = 1) {
	    std::print("\033[{}A", num);
	}
    void cursor_down(int num = 1) {
		std::print("\033[{}B", num);
	}

	size_t width() {
		winsize size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		return size.ws_col;
	}

	size_t height() {
		winsize size;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		return size.ws_row;
	}

};