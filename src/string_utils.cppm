/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/19 16:22:38
********************************************************************************/
module;
export module modforge.string_utils;
import std;

NAMESPACE_BEGIN
export namespace StringUtils {
    template <typename T>
    concept is_char = std::is_same_v<std::decay_t<T>, char>
                    || std::is_same_v<std::decay_t<T>, const char>
                    || std::is_same_v<std::decay_t<T>, wchar_t>
                    || std::is_same_v<std::decay_t<T>, const wchar_t>;

    template <typename T>
    concept is_string =
        std::is_same_v<std::decay_t<T>, std::string> ||
        std::is_same_v<std::decay_t<T>, std::string_view> ||
        std::is_same_v<std::decay_t<T>, char*> ||
        std::is_same_v<std::decay_t<T>, const char*> ||
        std::is_same_v<std::decay_t<T>, wchar_t*> ||
        std::is_same_v<std::decay_t<T>, const wchar_t*>;

    // 去除首尾空白（空格/制表/回车）
    constexpr std::string_view trim(std::string_view s) {
        auto b = s.find_first_not_of(" \t\r");
        if (b == std::string_view::npos) return std::string_view{};
        return s.substr(b, s.find_last_not_of(" \t\r") - b + 1);
    }

    // 字符串分割
    template <typename String>
        requires is_string<String> || is_char<String>
    std::vector<std::string> split(const std::string &extents, String delimiter) {
        auto view = extents
            | std::views::split(delimiter)
            | std::views::transform([](auto&& range) {
                return std::string(range.begin(), range.end());
            });
        return std::vector<std::string>(view.begin(), view.end());
    }

    // consteval 版本：编译期把字符串按分隔符拆成固定大小数组。
    // 注意不能返回 std::vector<std::string>——consteval 函数返回值必须是
    // 可常量初始化的字面量，堆分配（operator new）的容器不能作为 consteval 返回。
    // 返回 std::array<std::string_view, N>，结果可在 constexpr 上下文使用。
    // N 必须 >= 实际字段数（多余槽位为空 string_view）。
    template <std::size_t N>
    consteval std::array<std::string_view, N> split_to_array(std::string_view extents,
                                                             char delimiter) {
        std::array<std::string_view, N> ret{};
        std::size_t idx = 0, start = 0;
        for (std::size_t i = 0; i <= extents.size(); ++i) {
            if (i == extents.size() || extents[i] == delimiter) {
                if (idx < N)
                    ret[idx++] = extents.substr(start, i - start);
                start = i + 1;
            }
        }
        return ret;
    }

    // 数组重载：直接接收 const char[N]（如 #embed 数组），按分隔符拆成 array<string_view, OutN>。
    // 在 constexpr（非 consteval）调用方里传形参数组可用（实测：ConfigGenerator 复用此版）。
    template <std::size_t OutN, std::size_t N>
    consteval std::array<std::string_view, OutN> split_to_array(const char (&extents)[N],
                                                                char delimiter) {
        std::array<std::string_view, OutN> ret{};
        std::size_t idx = 0, start = 0;
        for (std::size_t i = 0; i < N; ++i) {
            if (extents[i] == delimiter) {
                if (idx < OutN)
                    ret[idx++] = std::string_view(&extents[start], i - start);
                start = i + 1;
            }
        }
        if (idx < OutN)
            ret[idx++] = std::string_view(&extents[start], N - 1 - start);
        return ret;
    }

    template <typename ...Strings, typename Delimiter>
        requires ((is_string<Strings> || is_char<Strings>) && ...) && (is_string<Delimiter> || is_char<Delimiter>)
    std::string join(Delimiter delimiter, Strings ...strings) {
        std::string ret;
        bool first = true;
        auto lam = [&](const auto &s) {
            if (!first)
                ret += delimiter;
            ret += s;
            first = false;
        };

        (..., lam(strings));
        return ret;
    }


}

NAMESPACE_END
