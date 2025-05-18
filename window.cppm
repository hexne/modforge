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

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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

    void draw_line(std::tuple<int, int> pos1, std::tuple<int, int> pos2, std::tuple<int, int, int> = { 0, 0, 0 });
    void draw_rect(std::tuple<int, int> pos1, std::tuple<int, int> pos2, std::tuple<int, int, int> = { 0, 0, 0 });
    void draw_text(std::tuple<int, int> pos, int pt, const std::string &text, std::tuple<int, int, int> = { 0, 0, 0 });

};

module :private;
#ifdef _WIN32

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Window *p{};
    // if (uMsg == WM_NCCREATE) {
    //     CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
    //     p = (Window*)pCreate->lpCreateParams; // 获取 this 指针
    //     SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p); // 存储到 USERDATA
    //     return TRUE; // 允许窗口继续创建
    // }

    auto pthis = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA); // 获取 this 指针
    if (!pthis) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }


    switch (uMsg) {
    case WM_SIZE:
        pthis->width_ = LOWORD(lParam);
        pthis->height_ = HIWORD(lParam);
        if (pthis->resize_callback)
            pthis->resize_callback();
        break;
    case WM_MOVE:
            if (pthis->move_callback)
                pthis->move_callback();
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

Window::Window(const std::string &title) : Window(600, 400, "window") {  }

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

        SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

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

void Window::draw_line(std::tuple<int, int> pos1, std::tuple<int, int> pos2, std::tuple<int, int, int> rgb) {
    HDC hdc = GetDC(hwnd_);
    if (!hdc) return;

    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(std::get<0>(rgb), std::get<1>(rgb), std::get<2>(rgb))); // Black pen
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    MoveToEx(hdc, std::get<0>(pos1), std::get<1>(pos1), nullptr);
    LineTo(hdc, std::get<0>(pos2), std::get<1>(pos2));

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    ReleaseDC(hwnd_, hdc);
}

void Window::draw_rect(std::tuple<int, int> pos1, std::tuple<int, int> pos2, std::tuple<int, int, int> rgb) {
    HDC hdc = GetDC(hwnd_);
    if (!hdc) return;

    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(std::get<0>(rgb), std::get<1>(rgb), std::get<2>(rgb))); // Black pen
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH)); // No fill

    Rectangle(hdc,
              std::get<0>(pos1), std::get<1>(pos1),
              std::get<0>(pos2), std::get<1>(pos2));

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    ReleaseDC(hwnd_, hdc);
}

void Window::draw_text(std::tuple<int, int> pos, int pt, const std::string& text, std::tuple<int, int, int> rgb) {
    HDC hdc = GetDC(hwnd_);
    if (!hdc) return;

    HFONT hFont = CreateFont(
        -MulDiv(pt, GetDeviceCaps(hdc, LOGPIXELSY), 72), // Height
        0, // Width
        0, // Escapement
        0, // Orientation
        FW_NORMAL, // Weight
        FALSE, // Italic
        FALSE, // Underline
        FALSE, // Strikeout
        DEFAULT_CHARSET,
        OUT_OUTLINE_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        VARIABLE_PITCH,
        TEXT("Arial")); // Font family

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(std::get<0>(rgb), std::get<1>(rgb), std::get<2>(rgb))); // Black text
    SetBkMode(hdc, TRANSPARENT); // Transparent background

    TextOutA(hdc, std::get<0>(pos), std::get<1>(pos), text.c_str(), (int)text.length());

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    ReleaseDC(hwnd_, hdc);
}
#elif __linux__


#endif