/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/13 14:01:03
********************************************************************************/

module;
export module modforge.utils;
import std;

export NAMESPACE_BEGIN
/** @brief 用 std::format 格式化消息构造 runtime_error
 *  @param fmt 格式化字符串
 *  @param vs 待填充的参数
 *  @return 构造好的 runtime_error
 */
template<typename ...Ts>
std::runtime_error
    format_runtime_error(std::format_string<Ts...>&& fmt, Ts&&... vs) noexcept {
    return std::runtime_error{std::format<Ts...>(fmt, std::forward<Ts>(vs)...)};
}

NAMESPACE_END