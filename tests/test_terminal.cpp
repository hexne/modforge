import std;
import modforge.terminal;

int test_terminal() {
    const auto w = modforge::terminal::width();
    const auto h = modforge::terminal::height();

    if (w < 0 || h < 0) return 1;

    modforge::terminal::hide_cursor();
    modforge::terminal::show_cursor();
    modforge::terminal::clear();

    modforge::terminal::cursor_x(0);
    modforge::terminal::cursor_y(0);
    modforge::terminal::cursor(0, 0);

    return 0;
}
