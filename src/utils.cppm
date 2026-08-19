/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/13 14:01:03
********************************************************************************/

module;
export module modforge.utils;
import std;

export NAMESPACE_BEGIN
// 封装异常， 直接format
template<typename ...Ts>
std::runtime_error
    format_runtime_error(std::format_string<Ts...>&& fmt, Ts&&... vs) noexcept {
    return std::runtime_error{std::format<Ts...>(fmt, std::forward<Ts>(vs)...)};
}

NAMESPACE_END