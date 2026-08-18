/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/13 14:01:03
********************************************************************************/

module;
export module utils;
import std;

export NAMESPACE_BEGIN
// 封装异常， 直接format
template<typename ...Ts>
std::runtime_error
    format_runtime_error(std::format_string<Ts...>&& fmt, Ts&&... vs) noexcept {
    return std::runtime_error{std::format<Ts...>(fmt, std::forward<Ts>(vs)...)};
}


// 字符串分割
std::vector<std::string> split(const std::string &extents, char split_char = ';') {
    auto view = extents
        | std::views::split(split_char)
        | std::views::transform([](auto&& range) {
            return std::string(range.begin(), range.end());
        });
    return std::vector<std::string>(view.begin(), view.end());
}

template <typename T>
concept is_string =
    std::is_same_v<std::decay_t<T>, std::string> ||
    std::is_same_v<std::decay_t<T>, std::string_view> ||
    std::is_same_v<std::decay_t<T>, char*> ||
    std::is_same_v<std::decay_t<T>, const char*> ||
    std::is_same_v<std::decay_t<T>, wchar_t*> ||
    std::is_same_v<std::decay_t<T>, const wchar_t*>;


NAMESPACE_END