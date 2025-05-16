/*******************************************************************************
* @Author : hexne
* @Data   : 2025/03/09 11:08
*******************************************************************************/

module;

#ifdef _WIN32
#include <windows.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#elif __linux__
#include <mutex>
#include <thread>
#include <map>
#include <string>
#include <condition_variable>

#include <iostream>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

export module modforge.keyevent;

#ifdef _WIN32

using std::cout, std::endl;
LRESULT CALLBACK KeyCallBack(int nCode,WPARAM wParam,LPARAM lParam) {
	KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam;		// 包含低级键盘输入事件信息
	if (ks->flags == 128 || ks->flags == 129)
	{
		// 监控键盘
		switch (ks->vkCode) {
		case 0x30: case 0x60:
			cout << "检测到按键：" << "0" << endl;
			break;
		case 0x31: case 0x61:
			cout << "检测到按键：" << "1" << endl;
			break;
		case 0x32: case 0x62:
			cout << "检测到按键：" << "2" << endl;
			break;
		case 0x33: case 0x63:
			cout << "检测到按键：" << "3" << endl;
			break;
		case 0x34: case 0x64:
			cout << "检测到按键：" << "4" << endl;
			break;
		case 0x35: case 0x65:
			cout << "检测到按键：" << "5" << endl;
			break;
		case 0x36: case 0x66:
			cout << "检测到按键：" << "6" << endl;
			break;
		case 0x37: case 0x67:
			cout << "检测到按键：" << "7" << endl;
			break;
		case 0x38: case 0x68:
			cout << "检测到按键：" << "8" << endl;
			break;
		case 0x39: case 0x69:
			cout << "检测到按键：" << "9" << endl;
			break;
		case 0x41:
			cout << "检测到按键：" << "A" << endl;
			break;
		case 0x42:
			cout << "检测到按键：" << "B" << endl;
			break;
		case 0x43:
			cout << "检测到按键：" << "C" << endl;
			break;
		case 0x44:
			cout << "检测到按键：" << "D" << endl;
			break;
		case 0x45:
			cout << "检测到按键：" << "E" << endl;
			break;
		case 0x46:
			cout << "检测到按键：" << "F" << endl;
			break;
		case 0x47:
			cout << "检测到按键：" << "G" << endl;
			break;
		case 0x48:
			cout << "检测到按键：" << "H" << endl;
			break;
		case 0x49:
			cout << "检测到按键：" << "I" << endl;
			break;
		case 0x4A:
			cout << "检测到按键：" << "J" << endl;
			break;
		case 0x4B:
			cout << "检测到按键：" << "K" << endl;
			break;
		case 0x4C:
			cout << "检测到按键：" << "L" << endl;
			break;
		case 0x4D:
			cout << "检测到按键：" << "M" << endl;
			break;
		case 0x4E:
			cout << "检测到按键：" << "N" << endl;
			break;
		case 0x4F:
			cout << "检测到按键：" << "O" << endl;
			break;
		case 0x50:
			cout << "检测到按键：" << "P" << endl;
			break;
		case 0x51:
			cout << "检测到按键：" << "Q" << endl;
			break;
		case 0x52:
			cout << "检测到按键：" << "R" << endl;
			break;
		case 0x53:
			cout << "检测到按键：" << "S" << endl;
			break;
		case 0x54:
			cout << "检测到按键：" << "T" << endl;
			break;
		case 0x55:
			cout << "检测到按键：" << "U" << endl;
			break;
		case 0x56:
			cout << "检测到按键：" << "V" << endl;
			break;
		case 0x57:
			cout << "检测到按键：" << "W" << endl;
			break;
		case 0x58:
			cout << "检测到按键：" << "X" << endl;
			break;
		case 0x59:
			cout << "检测到按键：" << "Y" << endl;
			break;
		case 0x5A:
			cout << "检测到按键：" << "Z" << endl;
			break;
		case 0x6A:
			cout << "检测到按键：" << "*" << endl;
			break;
		case 0x6B:
			cout << "检测到按键：" << "+" << endl;
			break;
		case 0x6D:
			cout << "检测到按键：" << "-" << endl;
			break;
		case 0x6E:
			cout << "检测到按键：" << "." << endl;
			break;
		case 0x6F:
			cout << "检测到按键：" << "/" << endl;
			break;
		case 0x0D:
			cout << "检测到按键：" << "Enter" << endl;
			break;
		case 0xA0: case 0xA1:
			cout << "检测到按键：" << "Shift" << endl;
			break;
		case 0x08:
			cout << "检测到按键：" << "Backspace" << endl;
			break;
		case 0x20:
			cout << "检测到按键：" << "Space" << endl;
			break;
		}

		//return 1;		// 使按键失效
	}

	// 将消息传递给钩子链中的下一个钩子
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}




export class KeyEventListener {
	std::mutex mutex;
	std::condition_variable cv;
	std::thread thread;
	bool listen_key = false;
	HHOOK hook;

public:
	KeyEventListener() {
		thread = std::thread(&KeyEventListener::Listen,this);
		thread.detach();
	}
	void Listen() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			listen_key = true;
		}
		cv.notify_one();

		hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyCallBack,0,0);
		MSG msg;
		while (listen_key && GetMessage(&msg,nullptr,0,0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return;
	}
	void DisListen() {
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [&] {return listen_key; });
		listen_key = false;
		UnhookWindowsHookEx(hook);
	}

};


LRESULT CursorCallBack(int nCode, WPARAM wParam, LPARAM lParam) {
	MSLLHOOKSTRUCT* pMouseSttruct = (MSLLHOOKSTRUCT*)lParam;
	if (nCode >= 0) {
		if (pMouseSttruct != nullptr) {
			std::cout << pMouseSttruct->pt.x << " " << pMouseSttruct->pt.y << std::endl;
		}
		switch (wParam) {
			case WM_LBUTTONDOWN:
				printf("Left button down\n");
				break;
			case WM_LBUTTONUP:
				printf("Left button up\n");
				break;
			case WM_RBUTTONDOWN:
				printf("Right button down\n");
				break;
			case WM_RBUTTONUP:
				printf("Right button up\n");
				break;
			case WM_MOUSEMOVE:
				printf("Mouse move\n");
				break;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);

}

export class CursorEventListener {
	std::mutex mutex;
	std::condition_variable cv;
	std::thread thread;
	bool listen_cursor = false;
	HHOOK hook;
public:


	CursorEventListener() {
		thread = std::thread(&CursorEventListener::Listen, this);
		thread.detach();
	}
	void Listen() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			listen_cursor = true;
		}
		cv.notify_one();

		hook = SetWindowsHookEx(WH_MOUSE_LL, CursorCallBack, 0, 0);
		MSG msg;
		while (listen_cursor && GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return;
	}
	void DisListen() {
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [&] {return listen_cursor; });
		listen_cursor = false;
		UnhookWindowsHookEx(hook);
	}

};



#elif __linux
// 
// // linux X API版本
// /*******************************************************************************
//  * Author : yongheng
//  * Data   : 2023/10/26 22:27
// *******************************************************************************/
// 
// 
// auto RegisterKey(Display *display,Window root) {
//     std::map<std::string,int> keys;
// #define TOCODE(key) keys[#key] = XKeysymToKeycode(display,XK_##key)
//     TOCODE(a);
//     TOCODE(b);
//     TOCODE(c);
//     TOCODE(d);
//     TOCODE(e);
//     TOCODE(f);
//     TOCODE(g);
//     TOCODE(h);
//     TOCODE(i);
//     TOCODE(j);
//     TOCODE(k);
//     TOCODE(l);
//     TOCODE(m);
//     TOCODE(n);
//     TOCODE(o);
//     TOCODE(p);
//     TOCODE(q);
//     TOCODE(r);
//     TOCODE(s);
//     TOCODE(t);
//     TOCODE(u);
//     TOCODE(v);
//     TOCODE(w);
//     TOCODE(x);
//     TOCODE(y);
//     TOCODE(z);
// 
//     TOCODE(A);
//     TOCODE(B);
//     TOCODE(C);
//     TOCODE(D);
//     TOCODE(E);
//     TOCODE(F);
//     TOCODE(G);
//     TOCODE(H);
//     TOCODE(I);
//     TOCODE(J);
//     TOCODE(K);
//     TOCODE(L);
//     TOCODE(M);
//     TOCODE(N);
//     TOCODE(O);
//     TOCODE(P);
//     TOCODE(Q);
//     TOCODE(R);
//     TOCODE(S);
//     TOCODE(T);
//     TOCODE(U);
//     TOCODE(V);
//     TOCODE(W);
//     TOCODE(X);
//     TOCODE(Y);
//     TOCODE(Z);
//     for (const auto& p : keys) {
//         XGrabKey(display, p.second, 0, root, True, GrabModeAsync, GrabModeAsync);
//     }
// 
//     return keys;
// #undef TOCODE
// }
// 
// void DisRegisterKey(Display *display,Window root,std::map<std::string,int> &keys) {
//     for (const auto &p : keys) {
//         XUngrabKey(display,p.second,0,root);
//     }
// }
// 
// export class KeyEventListener {
//     unsigned long lastReleaseTime = 0;
//     Window root;
//     XEvent event;
//     Display *display;
//     std::map<std::string,int> keys;
// 
//     bool listen = false;
//     std::mutex mutex;
//     std::condition_variable cv;
//     std::thread thread;
// 
// public:
//     KeyEventListener() {
//         thread = std::thread(&KeyEventListener::Listen,this);
//         thread.detach();
//     }
//     void Listen() {
//         {
//             std::lock_guard<std::mutex> lock(mutex);
//             listen = true;
//         }
// 
//         display = XOpenDisplay(0);
// 
//         root = DefaultRootWindow(display);
// 
//         int F4 = XKeysymToKeycode(display, XStringToKeysym("F4"));
//         std::cout << F4 << std::endl;
//         int F6 = XKeysymToKeycode(display, XStringToKeysym("F6"));
// 
//         //Register the keys
//         XGrabKey(display, F4, 0, root, True, GrabModeAsync, GrabModeAsync);
//         XGrabKey(display, F6, 0, root, True, GrabModeAsync, GrabModeAsync);
// 
//         keys = RegisterKey(display,root);
// 
//         while (listen) {
// 
//             XNextEvent(display,&event);
//             if (event.type == KeyPress && event.xkey.time != lastReleaseTime) {
//                 if (event.xkey.keycode == F4)
//                     return;
//                 if (event.xkey.keycode == keys["a"])
//                     std::cout << "a" << std::endl;
// 
//                 else if (event.xkey.keycode == keys["b"])
//                     std::cout << "b" << std::endl;
//                 else if (event.xkey.keycode == keys["c"])
//                     std::cout << "c" << std::endl;
//                 else if (event.xkey.keycode == F6)
//                     std::cout << "F6" << std::endl;
//                 //do something...
//             }
//             else if (event.type == KeyRelease) {
//                 lastReleaseTime = event.xkey.time;
//             }
//         }
//     }
// 
//     void DisListen() {
//         std::unique_lock<std::mutex> lock(mutex);
//         cv.wait(lock,[&]{return listen;});
//         listen = false;
//         DisRegisterKey(display,root,keys);
//     }
// 
// };
#endif