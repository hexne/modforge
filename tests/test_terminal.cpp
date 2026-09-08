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

    // 相对上/下移动（默认 1 行与显式行数）
    modforge::terminal::cursor_up();
    modforge::terminal::cursor_down();
    modforge::terminal::cursor_up(2);
    modforge::terminal::cursor_down(2);

    return 0;
}
