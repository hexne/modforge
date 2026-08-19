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
