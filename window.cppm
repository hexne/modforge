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

    void set_attributes(float attributes);

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

void Window::set_attributes(float attributes) {
    if (attributes < 0 || attributes > 1)
        return;

    // 必须设置窗口为分层窗口才能使用透明度
    SetWindowLong(hwnd_, GWL_EXSTYLE,
                 GetWindowLong(hwnd_, GWL_EXSTYLE) | WS_EX_LAYERED);

    // 设置透明度 (0-255)
    BYTE alpha = static_cast<BYTE>(attributes * 255);
    SetLayeredWindowAttributes(hwnd_, 0, alpha, LWA_ALPHA);}

void Window::resize(int w, int h) {
    SetWindowPos(hwnd_, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
}

void Window::move_to(int x, int y) {
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void Window::draw_line(std::tuple<int, int> pos1, std::tuple<int, int> pos2) {

}
void Window::draw_rect(std::tuple<int, int> pos1, std::tuple<int, int> pos2) {

}
#elif __linux__
#endif