/*******************************************************************************
* @Author : hexne
* @Data   : 2025/03/09 11:08
*******************************************************************************/

module;
#include <cctype>
#include <ranges>
#include <thread>
#include <string>
#include <functional>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

export module modforge.keyevent;

export
struct Key {

    static std::array<std::string, 256> key_map;

    std::vector<int> keys;
    std::function<void()> call_back;
    bool is_callbacked {};

	static std::vector<int> parse(const std::string &keys);

    Key(const std::vector<int> &keys) : keys(keys) {  }


	// <W>, <Ctrl-W>, <Ctrl-Shift-W>, <Ctrl-Alt-W>, <Ctrl-Shift-Alt-W>
	// W, Ctrl-W, Ctrl-Shift-W, Ctrl-Alt-W, Ctrl-Shift-Alt-W
	Key(std::string keys);

    // 按下
    void down();

    // 抬起
    void up();

    // 按下并抬起
    // 对于组合键，是全部按下后释放
    void press(int count = 1);

};

export
class KeyEvent {

	std::jthread listen_thread_;

    std::vector<Key> keys;

	void listen();

public:

    void add_key(Key key) {
        keys.emplace_back(std::move(key));
    }

	KeyEvent();
	~KeyEvent();


};





module : private;


std::array<std::string, 256> Key::key_map = {
    "",

    /*
     * 0x01 (1) - VK_LBUTTON     鼠标左键
     * 0x02 (2) - VK_RBUTTON     鼠标右键
     * 0x03 (3) - VK_CANCEL      Break/Ctrl+Pause
     * 0x04 (4) - VK_MBUTTON     鼠标中键
     * 0x05 (5) - VK_XBUTTON1    鼠标X1键
     * 0x06 (6) - VK_XBUTTON2    鼠标X2键
     */
    "leftmouse", "rightmouse", "cancel", "middlemouse", "mousebutton1", "mousebutton2", "",

    /*
     * 0x07 (7) - 未定义
     * 0x08 (8) - VK_BACK        Backspace键
     * 0x09 (9) - VK_TAB         Tab键
     * 0x0A (10)- 未定义
     * 0x0B (11)- 未定义
     * 0x0C (12)- VK_CLEAR       小键盘5 (NumLock关闭时)
     * 0x0D (13)- VK_RETURN      Enter键
     */
    "backspace", "tab", "", "", "", "enter", "", "",

    /*
     * 0x0E (14)- 未定义
     * 0x0F (15)- 未定义
     * 0x10 (16)- VK_SHIFT       Shift键
     * 0x11 (17)- VK_CONTROL     Ctrl键
     * 0x12 (18)- VK_MENU        Alt键
     * 0x13 (19)- VK_PAUSE       Pause键
     * 0x14 (20)- VK_CAPITAL     Caps Lock键
     */
    "shift", "ctrl", "alt", "pause", "capslock", "", "", "", "", "", "",
    /*
     * 0x15 (21)- 未定义
     * 0x16 (22)- 未定义
     * 0x17 (23)- 未定义
     * 0x18 (24)- 未定义
     * 0x19 (25)- 未定义
     * 0x1A (26)- 未定义
     * 0x1B (27)- VK_ESCAPE      Esc键
     */
    "esc", "", "", "", "",
    /*
     * 0x1C (28)- 未定义
     * 0x1D (29)- 未定义
     * 0x1E (30)- 未定义
     * 0x1F (31)- 未定义
     * 0x20 (32)- VK_SPACE       空格键
     * 0x21 (33)- VK_PRIOR       Page Up键
     * 0x22 (34)- VK_NEXT        Page Down键
     * 0x23 (35)- VK_END         End键
     * 0x24 (36)- VK_HOME        Home键
     * 0x25 (37)- VK_LEFT        左箭头键
     * 0x26 (38)- VK_UP          上箭头键
     * 0x27 (39)- VK_RIGHT       右箭头键
     * 0x28 (40)- VK_DOWN        下箭头键
     */
    "space", "pageup", "pagedown", "end", "home", "left", "up", "right", "down",

    /*
     * 0x29 (41)- VK_SELECT      Select键
     * 0x2A (42)- VK_PRINT       Print键
     * 0x2B (43)- VK_EXECUTE     Execute键
     * 0x2C (44)- VK_SNAPSHOT    Print Screen键
     * 0x2D (45)- VK_INSERT      Insert键
     * 0x2E (46)- VK_DELETE      Delete键
     * 0x2F (47)- VK_HELP        Help键
     */
    "select", "print", "execute", "printscreen", "insert", "delete", "help",

    // 数字键0-9 (0x30-0x39)
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "", "",

    // 字母键A-Z (0x41-0x5A)
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",

    /*
     * 0x5B (91)- VK_LWIN        左Windows键
     * 0x5C (92)- VK_RWIN        右Windows键
     * 0x5D (93)- VK_APPS        应用程序键
     */
    "leftwin", "rightwin", "apps", "", "",

    // 小键盘键 (0x60-0x6F)
    "num0", "num1", "num2", "num3", "num4", "num5", "num6", "num7", "num8", "num9",
    "num*", "num+", "", "num-", "num.", "num/",

    // 功能键F1-F24 (0x70-0x87)
    "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10",
    "f11", "f12", "f13", "f14", "f15", "f16", "f17", "f18", "f19", "f20",
    "f21", "f22", "f23", "f24", "", "", "", "", "", "", "", "",

    /*
     * 0x90 (144)- VK_NUMLOCK     Num Lock键
     * 0x91 (145)- VK_SCROLL      Scroll Lock键
     */
    "numlock", "scrolllock", "", "", "", "", "", "", "", "", "", "", "", "", "", "",

    // 左右修饰键 (0xA0-0xA5)
    "leftshift", "rightshift", "leftctrl", "rightctrl", "leftalt", "rightalt",
    };


#ifdef _WIN32
std::vector<int> Key::parse(const std::string &keys) {
    auto split_keys = keys
            |   std::ranges::views::split('-')
            |   std::ranges::views::transform([](auto&& range) {
                    return std::string(range.begin(), range.end());
                })
            |   std::ranges::views::transform([](const std::string& str) {
                    auto pos = std::ranges::find(key_map, str);
                    if (pos == key_map.end())
                        throw std::runtime_error("Invalid key: " + str);
                    return pos - key_map.begin();
                });


    return { split_keys.begin(), split_keys.end() };
}


Key::Key(std::string keys) {

    for (auto& ch : keys) {
        if (std::isupper(ch))
            ch = std::tolower(ch);
    }
    if (keys.front() == '<' && keys.back() == '>')
        keys = std::string(keys.begin() + 1, keys.end() - 1);

    this->keys = parse(keys);
}

// 按下
void Key::down() {
    for (const auto &key : keys)
        keybd_event(key, 0, 0, 0);
}

// 抬起
void Key::up() {
    for (const auto &key : keys)
        keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
}

// 按下并抬起
// 对于组合键，是全部按下后释放
void Key::press(int count = 1) {
    while (count--) {
        down();
        up();
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

};



KeyEvent::KeyEvent() {

	listen_thread_ = std::jthread(&KeyEvent::listen, this);

}

KeyEvent::~KeyEvent() {
	if (listen_thread_.joinable()) {
		listen_thread_.request_stop();
		listen_thread_.join();
	}
}

void KeyEvent::listen() {

	while (!listen_thread_.get_stop_token().stop_requested()) {
        std::array<bool, 256> key_status {};
        for (int i = 0;i < key_status.size(); ++i) {
            if (!(GetAsyncKeyState(i) & 0x8000))
                continue;
            key_status[i] = true;
        }

        for (auto& key : keys) {
            bool flag = true;
            for (auto key_id : key.keys) {
                if (!key_status[key_id]) {
                    flag = false;
                    key.is_callbacked = false;
                }
            }

            if (flag && !key.is_callbacked) {
                key.call_back();
                key.is_callbacked = true;
            }
        }

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	}

}


#elif __linux__

[[deprecated("linux is not supported.")]]
KeyEvent::KeyEvent() {
    throw std::runtime_error("linux is not supported.");
}

#endif
