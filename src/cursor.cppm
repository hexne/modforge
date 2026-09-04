/*******************************************************************************
* @Author : hexne
* @Data   : 2023/5/28
*******************************************************************************/

module;
#ifdef _WIN32
#include <Windows.h>
#elif __linux__
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
	friend std::ostream& operator << (std::ostream& out, const CursorPos& pos) {
		out << pos.x << ' ' << pos.y;
		return out;
	}
};

export
class Cursor {
    std::thread listen_click_thread_;
    bool run_flag_ = true;
    std::mutex thread_mutex_;
    std::function<void()> left_click_callback_;
    std::function<void()> right_click_callback_;
    std::function<void()> move_callback_;

    CursorPos old_cursor_pos_;
    CursorPos cursor_pos;

    bool get_key_state(int key);
    void listen_event();
public:
    Cursor();
    Cursor(CursorPos);
	CursorPos get_cursor_pos();

    void click_left(std::size_t count = 1);
    void click_right(std::size_t count = 1);
	void click_left(const CursorPos &pos, std::size_t count = 1) {
		move_to(pos);
		click_left(count);
	}
	void click_right(const CursorPos &pos, std::size_t count = 1) {
    	move_to(pos);
    	click_right(count);
    }

	void set_click_left_callback(std::function<void()> callback) {
		left_click_callback_ = std::move(callback);
	}
	void set_click_right_callback(std::function<void()> callback) {
		right_click_callback_ = std::move(callback);
	}
	void set_move_callback(std::function<void()> callback) {
		move_callback_ = std::move(callback);
	}
    void move_to(const CursorPos&);
    void wheel(int value);

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
	// 注意：GetKeyState 返回的是调用线程消息队列处理过的输入状态，
	// listen_event 跑在无消息泵的裸线程上会恒为 0、点击永远检测不到。
	// GetAsyncKeyState 读全局实时状态，与消息队列无关，是监听线程唯一可靠的选择。
	return GetAsyncKeyState(key) & 0x8000;
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

void Cursor::click_left(std::size_t count) {
	while (count--)
		mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
void Cursor::click_right(std::size_t count) {
	while (count--)
		mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}

void Cursor::wheel(int value) {
	mouse_event(MOUSEEVENTF_WHEEL, 0, 0 , value * 120, 0);
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



#elif __linux__
#endif

