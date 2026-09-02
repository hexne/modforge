/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/12
********************************************************************************/
module;
export module modforge.args_parser;

import std;
import modforge.string_utils;
import modforge.utils;

NAMESPACE_BEGIN

export class ArgsParser;
export class InputArgs;

template <typename T>
constexpr bool is_support_type_v = StringUtils::is_string<T> || std::is_invocable_v<T, std::optional<InputArgs>>;

class InputArgs {

    std::vector<std::string> args{};
    std::string value{};
    friend class ArgsParser;

public:
    InputArgs() = default;
    explicit InputArgs(std::initializer_list<std::string> args) : args(args) {  }


    // 比较只涉及参数
    bool operator < (const InputArgs &other) const {
        if (args.size() != other.args.size())
            return args.size() < other.args.size();

        for (int i = 0;i < args.size(); ++i) {
            const auto &arg1 = args[i];
            const auto &arg2 = other.args[i];

            if (arg1 == arg2)
                continue;

            return arg1 < arg2;
        }
        return false;
    }

    std::string get_value() const {
        return value;
    }
    void set_value(std::string value) {
        this->value = value;
    }
};

// -s j k
// --short --join --kill
class ArgsParser {
    using Callback = std::function<void(std::optional<InputArgs>)>;
    std::vector<std::tuple<InputArgs, Callback>> support_args_;

    std::string remove_front_char(const std::string &arg) {
        auto pos = arg.find_last_of('-');
        if (pos == std::string::npos)
            throw format_runtime_error("No support args format {}", arg);

        return arg.substr(pos + 1);
    }
    // 这里的arg已经移除了-、--
    std::tuple<InputArgs&, Callback&> find_args(const std::string& cur_arg) {

        auto it = std::ranges::find_if(support_args_, [&cur_arg](const auto &arg) {
            auto &[input_args, callback] = arg;
            return std::ranges::any_of(input_args.args, [&cur_arg](const std::string &s) {
                return cur_arg == s;
            });
        });

        if (it == support_args_.end())
            throw format_runtime_error("No support args {}", cur_arg);

        return *it;
    }

    template <typename Tuple, std::size_t... index>
    void add_args_impl(Tuple &&tuple, std::index_sequence<index...>) {
        auto &callback = std::get<sizeof...(index)>(tuple);
        InputArgs input_args { remove_front_char(std::get<index>(tuple)) ... };
        support_args_.emplace_back(std::move(input_args),
            std::function<void(std::optional<InputArgs>)>(std::forward<decltype(callback)>(callback)));
    }

    template <typename Tuple, std::size_t... index>
    void add_flag_impl(Tuple &&tuple, std::index_sequence<index...>) {
        auto &obj = std::get<sizeof...(index)>(tuple);   // obj 是成员引用
        using MemberType = std::remove_reference_t<decltype(obj)>;
        (add_args(std::get<index>(tuple), [&obj](std::optional<InputArgs> value) {
            if (value) {
                // --flag=value：解析并赋给成员
                std::string_view s = value->get_value();
                if constexpr (std::is_same_v<MemberType, std::string>)
                    obj = std::string(s);
                else if constexpr (std::is_same_v<MemberType, bool>)
                    obj = (s == "true" || s == "1");
                else if constexpr (std::is_same_v<MemberType, int>)
                    obj = std::stoi(std::string(s));
                else if constexpr (std::is_same_v<MemberType, double>)
                    obj = std::stod(std::string(s));
                else static_assert(sizeof(MemberType) == 0, "unsupported member type");
            } else if constexpr (std::is_same_v<MemberType, bool>) {
                obj = true;   // -la / --long 无值：bool 置 true，其他类型忽略
            }
        }), ...);
    }

public:
    template <typename ...Args>
        requires (is_support_type_v<Args> && ...)
    void add_args(Args &&...args) {
        constexpr auto args_size = sizeof...(Args);
        auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        add_args_impl(std::move(tuple), std::make_index_sequence<args_size - 1>{});
    }

    template <typename ...Args>
        requires ((std::is_same_v<std::remove_reference_t<Args>, int>
                || std::is_same_v<std::remove_reference_t<Args>, double>
                || std::is_same_v<std::remove_reference_t<Args>, bool>
                || is_support_type_v<Args>) && ...)
    void add_flag(Args &&...args) {
        constexpr auto args_size = sizeof...(Args);
        auto tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        add_flag_impl(std::move(tuple), std::make_index_sequence<args_size - 1>{});
    }

    // -sjk
    // -s=value
    // --short=value
    void analysis(int argc, char *argv[]) {
        std::vector<std::string> input_args(argv + 1, argv + argc);
        for (const auto &arg : input_args) {
            // 赋值
            if (arg.find('=') != std::string::npos) {
                auto removed_front_arg = remove_front_char(arg);
                auto tuple = StringUtils::split(removed_front_arg, '=');

                if (tuple.size() != 2)
                    throw format_runtime_error("no support args format {}", arg);

                auto find_res = find_args(tuple.front());

                auto &[input_arg, callback] = find_res;
                input_arg.value = tuple.back();

                callback(input_arg);
            }
            // 长指令
            else if (arg.find("--") != std::string::npos) {
                auto cur_arg = remove_front_char(arg);
                auto find_res = find_args(cur_arg);

                auto &[input_arg, callback] = find_res;

                callback(std::nullopt);
            }
            // 短指令
            else if (arg.find('-') != std::string::npos) {
                auto args = remove_front_char(arg);
                for (auto ch : args) {
                    auto find_res = find_args(std::string(1, ch));
                    auto &[input_arg, callback] = find_res;
                    callback(std::nullopt);
                }
            }
            else {
                throw format_runtime_error("no support args format {}", arg);
            }
        }
    }
};

NAMESPACE_END