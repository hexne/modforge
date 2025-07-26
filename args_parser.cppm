/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/11/19 21:15
*******************************************************************************/
export module args_parser;
import std;

template <typename T>
constexpr bool is_string_v = std::is_same_v<std::remove_cvref_t<T>, std::string>
                || std::is_same_v<char *, std::decay_t<T>>
                || std::is_same_v<char *, std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>>>;

template <typename T>
constexpr bool is_support_type_v = is_string_v<T> || std::is_invocable_v<T>;

struct Arg {
    std::vector<std::string> args;
    std::function<void()> callback;
    bool enable{};
    std::string value;
};

// 参数parser_callback : 是否在解析程序参数时调用相应的callback
export template <bool parser_callback = true>
class ArgsParser {
    std::vector<Arg> support_args_; // 支持的参数，通过add 添加
    std::vector<std::string> program_args_, other_arg_;

    template<std::size_t ... index>
    void add_arg_impl(auto && tuple, auto &&callback, std::index_sequence<index...>) {
        Arg arg = { .args = {std::get<index>(tuple) ... }};
        arg.callback = callback;
        support_args_.emplace_back(std::move(arg));
    }

    Arg &find_arg(const std::string& find_arg) {
        for (auto &arg : support_args_) {
            auto &[args, _, _, _] = arg;
            // 查找是否是短指令 或 长指令
            if (std::ranges::find(args, find_arg) != std::ranges::end(args))
                return arg;

            // 查找是否是传递的键值对类型的值
            for (const auto& cur_args : args ) {
                if (cur_args.find(find_arg) != std::string::npos)
                    return arg;
            }
        }
        // 都没有找到
        throw std::invalid_argument("No such argument: " + find_arg);
    }
public:

    ArgsParser() = default;

    ArgsParser(int argc, char *argv[]) {
        for (int i = 1;i < argc; ++i)
            program_args_.emplace_back(argv[i]);
    }

    // 如果最后一个参数是callback, 那么作为开关选项
    // 如果最后一个参数是字符串，那么作为值选项
    void add_args(auto&&... args) requires (is_support_type_v<decltype(args)> && ...) {
        auto tuple = std::forward_as_tuple(args...);
        auto back = std::get<sizeof...(args) - 1>(tuple);

        if constexpr (is_string_v<decltype(back)>) {
            (support_args_.emplace_back(Arg{.args = {args}}), ...);
        }
        else if constexpr (std::is_invocable_v<decltype(back)>) {
            add_arg_impl(tuple, back, std::make_index_sequence<sizeof...(args) - 1>());
        }
    }
    // 通过参数获取值或是否开关
    Arg operator[](const std::string &arg) {
        return find_arg(arg);
    }
    auto other_arg() {
        return other_arg_;
    }

    // 解析
    void parser() {
        // 对于短指令
        for (int i = 0; i < program_args_.size(); ++i) {
            std::string cur_arg = program_args_[i];

            // -a -ah -cmake-version=4.0.0
            if (cur_arg[0] == '-' && cur_arg[1] != '-') {
                // 是键值对
                if (auto pos = cur_arg.find('='); pos != std::string::npos) {
                    std::string key = std::string(cur_arg.begin() + 1, cur_arg.begin() + pos);
                    std::string value = std::string(cur_arg.begin() + pos + 1, cur_arg.end());
                    auto &arg = find_arg(key);
                    arg.enable = true;
                    arg.value = value;
                }
                // 是短指令
                else {
                    for (int ch_pos = 1; ch_pos < cur_arg.size(); ++ ch_pos) {
                        std::string ch = '-' + std::string(1, cur_arg[ch_pos]);
                        auto &arg = find_arg(ch);
                        arg.enable = true;
                        if (parser_callback)
                            arg.callback();
                    }
                }
            }
            // --all
            else if (cur_arg[0] == '-' && cur_arg[1] == '-') {
                auto &arg = find_arg(cur_arg);
                arg.enable = true;
                if (parser_callback)
                    arg.callback();
            }

            // 其他传入的值
            else {
                other_arg_.emplace_back(std::move(cur_arg));
            }
        }
    }
    ~ArgsParser() = default;
};