/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 21:43:20
********************************************************************************/

import std;
import modforge.console;

int test_console() {

    int w = Console::width();
    int h = Console::height();

    Console::cursor_up();
    Console::cursor_down();

    Console::hind_cursor();
    Console::show_cursor();


    return 0;
}