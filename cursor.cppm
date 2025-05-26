/*******************************************************************************
* @Author : hexne
* @Data   : 2023/5/28
*******************************************************************************/

module;

#include <functional>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

export module modforge.cursor_event;

export
struct CursorPos {
	int x = 0;
	int y = 0;
	bool operator==(const CursorPos& right) const {
		return x == right.x && y == right.y;
	}
};

export
struct Cursor {
    Cursor();
    void click_left(int count);
    void click_right(int count);

    CursorPos get_cursor_pos();
    void set_click_left_callback(std::function<void()>);
    void set_click_right_callback(std::function<void()>);
    void set_move_callback(std::function<void()>);
    void move_to(const CursorPos&);

    ~Cursor();
};

module : private;
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

class MouseWindows : MouseBase {
    CursorPos old_cursor_pos_;
    std::thread listen_click_thread_;
    bool run_flag_ = true;
    std::mutex thread_mutex_;
    std::function<void()> left_click_callback_;
    std::function<void()> right_click_callback_;
    std::function<void()> move_callback_;


    bool get_key_state(int key) {
        return GetKeyState(key) & 0x8000;
    }
    void listen_event();

public:
    CursorPos cursor_pos;
    MouseWindows() : MouseWindows(get_cursor_pos()) {  }
    explicit MouseWindows(const CursorPos cursor_pos) : cursor_pos(cursor_pos) {
        old_cursor_pos_ = cursor_pos;
        listen_click_thread_ = std::thread(&MouseWindows::listen_event, this);
    }
    ~MouseWindows() {
        {
            std::lock_guard<std::mutex> lock(thread_mutex_);
            run_flag_ = false;

        }
        listen_click_thread_.detach();

    }
    CursorPos get_cursor_pos() override {
        POINT p;
        GetCursorPos(&p);
        return { p.x, p.y };
    }
    void move_to(const CursorPos &pos) override {
        cursor_pos = pos;
        SetCursorPos(pos.x, pos.y);
    }
    void click_left(const CursorPos pos) {
        cursor_pos = pos;
        mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
    void click_left(int count = 1) override {
        while (count--)
            click_left(cursor_pos);
    }
    void click_right(const CursorPos pos) {
        cursor_pos = pos;
        mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
    }
    void click_right(int count = 1) override {
        while (count--)
            click_right(cursor_pos);
    }
    void wheel_up(int val = 100) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, val, 0);
    }
    void wheel_down(int val = -100) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, val, 0);
    }
    void set_click_left_callback(std::function<void()> callback) override {
        left_click_callback_ = std::move(callback);
    }
    void set_click_right_callback(std::function<void()> callback) override {
        right_click_callback_ = std::move(callback);
    }
    void set_move_callback(std::function<void()> callback) override {
        move_callback_ = std::move(callback);
    }


};

void MouseWindows::listen_event() {
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
Cursor::Cursor() = default;
#endif

