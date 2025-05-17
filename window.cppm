/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/17 02:12:33
********************************************************************************/
module;
#include <iostream>
#include <thread>
#include <functional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#elif __linux__
#endif

export module modforge.window;

export class Window {
    int width_ = 800, height_ = 600;
    bool create_finish_{};
    HWND hwnd_;

    std::jthread thread_;

public:
    std::function<void()> move_callback, action_callback, resize_callback;
    Window(const std::string &title);
    Window(int w, int h, const std::string &title);
    ~Window();

    void show();
    void hide();

    void show_titlebar();
    void hide_titlebar();

    void resize(int w, int h);
    void move_to(int x, int y);

    void draw_line(std::tuple<int, int> pos1, std::tuple<int, int> pos2);
    void draw_rect(std::tuple<int, int> pos1, std::tuple<int, int> pos2);

};

module :private;
#ifdef _WIN32

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

Window::Window(const std::string &title) {

}
Window::Window(int w, int h, const std::string &title) : width_(w), height_(h) {
    static bool class_registered = false;

    thread_ = std::jthread([this, title](std::stop_token token) {
        if (!class_registered) {
            WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
            wc.style         = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc   = WindowProc;
            wc.hInstance     = GetModuleHandle(nullptr);
            wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = "WindowClass";

            if (!RegisterClassEx(&wc))
                throw std::runtime_error("Failed to register window class");
            class_registered = true;
        }
        hwnd_ = CreateWindowEx(
                0, "WindowClass", title.data(),
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                width_, height_,
                nullptr, nullptr, GetModuleHandle(nullptr), this
            );
        if (!hwnd_)
            throw std::runtime_error("Failed to create window");

        create_finish_ = true;
        MSG msg = {};
        while (!token.stop_requested() && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        DestroyWindow(hwnd_);
        UnregisterClass("WindowClass", GetModuleHandle(nullptr));
    });

    while (!create_finish_)
        ;
    std::cout << "create_finish" << std::endl;
}
Window::~Window() {
    if (hwnd_) {
        SendMessage(hwnd_, WM_CLOSE, 0, 0);
        while (IsWindow(hwnd_)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }
}

void Window::show() {
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
}

void Window::hide() {
    ShowWindow(hwnd_, SW_HIDE);
    UpdateWindow(hwnd_);
}

void Window::show_titlebar() {
    SetWindowLongPtr(hwnd_, GWL_STYLE,
        GetWindowLongPtr(hwnd_, GWL_STYLE) |
        WS_OVERLAPPEDWINDOW);
    SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

void Window::hide_titlebar() {
    SetWindowLongPtr(hwnd_, GWL_STYLE,
        GetWindowLongPtr(hwnd_, GWL_STYLE) &
        ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME));
    SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}



void Window::resize(int w, int h) {
    if (!hwnd_)
        return;

    width_ = w;
    height_ = h;

    // Windows 实现
    RECT rc = {0, 0, w, h};

    // 计算需要调整的窗口尺寸（包含边框）
    AdjustWindowRectEx(&rc,
                     GetWindowLongPtr(hwnd_, GWL_STYLE),
                     FALSE,
                     GetWindowLongPtr(hwnd_, GWL_EXSTYLE));

    SetWindowPos(hwnd_,
                nullptr,
                0, 0,
                rc.right - rc.left,  // 实际窗口宽度
                rc.bottom - rc.top,  // 实际窗口高度
                SWP_NOZORDER | SWP_NOMOVE);

    // 触发重绘
    InvalidateRect(hwnd_, nullptr, TRUE);

    if (resize_callback)
        resize_callback();
}
void Window::move_to(int x, int y) {
    if (!hwnd_)
        return ;

    SetWindowPos(hwnd_,
                nullptr,
                x, y,
                0, 0,  // 保持当前尺寸
                SWP_NOZORDER | SWP_NOSIZE);

    if (move_callback)
        move_callback();
}

void Window::draw_line(std::tuple<int, int> pos1, std::tuple<int, int> pos2) {

}
void Window::draw_rect(std::tuple<int, int> pos1, std::tuple<int, int> pos2) {

}
#elif __linux__
#endif