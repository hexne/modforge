/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/09/22 12:49
*******************************************************************************/

module;
#ifdef _WIN32
#include <Windows.h>
#else
#endif

#include <functional>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include "tools.h"
export module Mouse;

struct MouseBase {
    std::jthread listen_thread_;
    virtual void click_left(int count) = 0;
    virtual void click_right(int count) = 0;

    virtual std::tuple<size_t,size_t> get_mouse_pos() = 0;
    virtual void set_click_left_callback(std::function<void()>) = 0;
    virtual void set_click_right_callback(std::function<void()>) = 0;

    virtual void move_to(int x,int y) = 0;

    virtual ~MouseBase() = default;
};
#ifdef _WIN32
class Mouse: public MouseBase {

public:
    void click_left(int count) {

    }
    void click_right(int count) {

    }

    std::tuple<size_t,size_t> get_mouse_pos() {

    }
    void set_click_left_callback(std::function<void()>) {

    }
    void set_click_right_callback(std::function<void()>) {

    }

    void move_to(int x,int y) {

    }
};
#elif __linux__
class Mouse: public MouseBase {

public:
    void click_left(int count) {

    }
    void click_right(int count) {

    }

    std::tuple<size_t,size_t> get_mouse_pos() {

    }
    void set_click_left_callback(std::function<void()>) {

    }
    void set_click_right_callback(std::function<void()>) {

    }

    void move_to(int x,int y) {

    }
};
#endif

export
NAMESPACE_BEGIN(nl)
#ifdef _WIN32
constexpr int LeftButton = VK_LBUTTON;
constexpr int RightButton = VK_RBUTTON;


bool GetKeyState(int key) {
    return GetKeyState(key) & 0x8000;
}
void SetCursorPos(int x,int y) {
    SetCursorPos(x , y);
}

void LeftClick() {
    mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void RightClick() {
    mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}

std::tuple<int,int> GetCursorPos() {
    POINT p;
    GetCursorPos(&p);
    return {p.x,p.y};
}


#else
#endif
//
//
//
//
//
// class ClickEvent {
//     bool state = false;
// public:
//     bool operator ()(const bool clicked) {
//         const bool ignore = clicked == state;
//         state = clicked;
//         if (clicked && !ignore)
//             return true;
//         return false;
//     }
// };
//
// class Mouse {
//
// public:
//     struct CursorPos {
//         int x = 0;
//         int y = 0;
//         bool operator==(const CursorPos& right) const {
//             return x == right.x && y == right.y;
//         }
//
//     };
//
// private:
//
//     CursorPos old_cursor_pos_;
//     std::thread listen_click_thread_;
//     bool run_flag_ = true;
//     std::mutex thread_mutex_;
//     std::function<void()> left_click_callback_;
//     std::function<void()> right_click_callback_;
//     std::function<void()> move_callback_;
//
//     static CursorPos get_current_cursor_pos() {
//         const auto pos = GetCursorPos();
//         return { std::get<0>(pos),std::get<1>(pos) };
//     }
//
//     void listen_event() {
//         ClickEvent right_click_event;
//         ClickEvent left_click_event;
//
//         while (true) {
//             {
//                 std::lock_guard lock(thread_mutex_);
//                 if (!run_flag_) {
//                     return;
//                 }
//
//             }
//             if (left_click_event(GetKeyState(LeftButton))) {
//                 if (left_click_callback_)
//                     left_click_callback_();
//             }
//             if (right_click_event(GetKeyState(RightButton))) {
//                 if (right_click_callback_)
//                     right_click_callback_();
//             }
//             if (const auto cur_cursor_pos = get_current_cursor_pos(); cur_cursor_pos != old_cursor_pos_) {
//                 old_cursor_pos_ = cursor_pos = cur_cursor_pos;
//
//                 if (move_callback_)
//                     move_callback_();
//             }
//
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//
//         }
//     }
// public:
//     CursorPos cursor_pos;
//     Mouse() : Mouse(get_current_cursor_pos()){  }
//     explicit Mouse(const CursorPos cursor_pos) : cursor_pos(cursor_pos) {
//         old_cursor_pos_ = cursor_pos;
//         listen_click_thread_ = std::thread(&Mouse::listen_event, this);
//     }
//     ~Mouse() {
//         {
//             std::lock_guard<std::mutex> lock(thread_mutex_);
//             run_flag_ = false;
//
//         }
//         listen_click_thread_.detach();
//
//     }
//     void move_to(const CursorPos pos) {
//         cursor_pos = pos;
//         SetCursorPos(pos.x , pos.y);//更改鼠标坐标
//     }
//     void lift_click(const CursorPos pos) {
//         cursor_pos = pos;
//         LeftClick();
//     }
//     void lift_click(int count = 1) {
//         while (count --)
//             lift_click(cursor_pos);
//     }
//     void right_click(const CursorPos pos) {
//         cursor_pos = pos;
//         RightClick();
//     }
//     void right_click(int count = 1) {
//         while (count --)
//             right_click(cursor_pos);
//     }
//
//     void set_left_click_callback(std::function<void()> callback) {
//         left_click_callback_ = std::move(callback);
//     }
//     void set_right_click_callback(std::function<void()> callback) {
//         right_click_callback_ = std::move(callback);
//     }
//     void set_move_callback(std::function<void()> callback) {
//         move_callback_ = std::move(callback);
//     }
//
//
// };
//
// std::ostream& operator << (std::ostream& out, const Mouse::CursorPos& pos) {
//     out << pos.x << ' ' << pos.y;
//     return out;
// }

NAMESPACE_END(nl)
