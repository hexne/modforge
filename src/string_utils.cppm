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

    /** @brief 去除首尾空白（空格/制表/回车）
     *  @param s 待处理的字符串视图
     *  @return 去除首尾空白后的视图（全空白时返回空）
     */
    constexpr std::string_view trim(std::string_view s) {
        auto b = s.find_first_not_of(" \t\r");
        if (b == std::string_view::npos) return std::string_view{};
        return s.substr(b, s.find_last_not_of(" \t\r") - b + 1);
    }

    /** @brief 按分隔符分割字符串（运行时）
     *  @param extents 待分割的字符串
     *  @param delimiter 分隔符，字符串或字符
     *  @return 分割结果
     */
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

    /** @brief 编译期分割为固定大小数组
     *  @param N 目标槽位数，须 >= 实际字段数（多余槽位为空 string_view）
     *  @param extents 待分割的字符串
     *  @param delimiter 分隔符字符
     *  @return 固定大小数组（consteval 不可返回 vector）
     */
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

    /** @brief 编译期分割重载：直接接收字符数组（如 #embed 数组）
     *  @param extents 待分割的字符数组
     *  @param delimiter 分隔符字符
     *  @return 固定大小数组（constexpr 形参数组可用）
     */
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

    /** @brief 用分隔符连接多个字符串
     *  @param delimiter 连接用的分隔符
     *  @param strings 待连接的字符串（可多个）
     *  @return 连接后的字符串
     */
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
