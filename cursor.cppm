/*******************************************************************************
* @Author : hexne
* @Data   : 2023/5/28
*******************************************************************************/

module;

#ifdef _WIN32
#include <Windows.h>
#endif

export module modforge.cursor;

import std;

export
struct CursorPos {
	int x = 0;
	int y = 0;
	bool operator==(const CursorPos& right) const {
		return x == right.x && y == right.y;
	}
};

export
class Cursor {
    CursorPos old_cursor_pos_;
    std::thread listen_click_thread_;
    bool run_flag_ = true;
    std::mutex thread_mutex_;
    std::function<void()> left_click_callback_;
    std::function<void()> right_click_callback_;
    std::function<void()> move_callback_;
    CursorPos cursor_pos;

    bool get_key_state(int key);
    void listen_event();
public:
    Cursor();
    Cursor(CursorPos);

    void click_left(int count);
    void click_left(const CursorPos);
    void click_right(int count);
    void click_right(const CursorPos);

    CursorPos get_cursor_pos();
    void set_click_left_callback(std::function<void()>);
    void set_click_right_callback(std::function<void()>);
    void set_move_callback(std::function<void()>);
    void move_to(const CursorPos&);
    void wheel_up(int val = 100);
    void wheel_down(int val = 100);

    ~Cursor();
};

#ifdef _WIN32
class ClickEvent {
    bool state = false;
public:
    bool operator ()(const bool clicked) {
        const bool ignore = clicked == state;
        state = clicked;
        if (clicked && !ignore)
            return true;
        return false;
    }
};


bool Cursor::get_key_state(int key) {
	return GetKeyState(key) & 0x8000;
}

Cursor::Cursor() : Cursor(get_cursor_pos()) { }

Cursor::Cursor(const CursorPos cursor_pos) : cursor_pos(cursor_pos) {
	old_cursor_pos_ = cursor_pos;
	listen_click_thread_ = std::thread(&Cursor::listen_event, this);
}
Cursor::~Cursor() {
	{
		std::lock_guard<std::mutex> lock(thread_mutex_);
		run_flag_ = false;

	}
	listen_click_thread_.detach();

}

CursorPos Cursor::get_cursor_pos() {
	POINT p;
	GetCursorPos(&p);
	return { p.x, p.y };
}
void Cursor::move_to(const CursorPos &pos) {
	cursor_pos = pos;
	SetCursorPos(pos.x, pos.y);
}
void Cursor::click_left(const CursorPos pos) {
	cursor_pos = pos;
	mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
void Cursor::click_left(int count = 1) {
	while (count--)
		click_left(cursor_pos);
}
void Cursor::click_right(const CursorPos pos) {
	cursor_pos = pos;
	mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}
void Cursor::click_right(int count = 1) {
	while (count--)
		click_right(cursor_pos);
}
void Cursor::wheel_up(int val) {
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, val, 0);
}
void Cursor::wheel_down(int val) {
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, val, 0);
}
void Cursor::set_click_left_callback(std::function<void()> callback) {
	left_click_callback_ = std::move(callback);
}
void Cursor::set_click_right_callback(std::function<void()> callback) {
	right_click_callback_ = std::move(callback);
}
void Cursor::set_move_callback(std::function<void()> callback) {
	move_callback_ = std::move(callback);
}

void Cursor::listen_event() {
    ClickEvent right_click_event;
    ClickEvent left_click_event;

    while (true) {
        {
            std::lock_guard lock(thread_mutex_);
            if (!run_flag_) {
                return;
            }

        }
        if (left_click_event(get_key_state(VK_LBUTTON))) {
            if (left_click_callback_)
                left_click_callback_();
        }
        if (right_click_event(get_key_state(VK_RBUTTON))) {
            if (right_click_callback_)
                right_click_callback_();
        }
        if (const auto cur_cursor_pos = get_cursor_pos(); cur_cursor_pos != old_cursor_pos_) {
            old_cursor_pos_ = cursor_pos = cur_cursor_pos;

            if (move_callback_)
                move_callback_();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
    }
}


std::ostream& operator << (std::ostream& out, const CursorPos& pos) {
    out << pos.x << ' ' << pos.y;
    return out;
}
#elif __linux__
[[deprecated("linux is not supported.")]]
Cursor::Cursor() {
    throw std::runtime_error("linux not supported");
}

[[deprecated("linux is not supported.")]]
Cursor::Cursor(CursorPos) {
    throw std::runtime_error("linux not supported");
}
#endif

