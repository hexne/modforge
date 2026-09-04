/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/31 17:57:03
********************************************************************************/

module;
export module modforge.config_generator;
import std;
import modforge.string_utils;
import modforge.utils;


NAMESPACE_BEGIN

consteval std::meta::info line_to_spec(std::string_view line) {
    auto eq = line.find('=');
    std::string_view name = StringUtils::trim(line.substr(0, eq));
    std::string_view value = StringUtils::trim(line.substr(eq + 1));

    std::meta::info type;
    if (value.find('"') != std::string_view::npos) {
        type = ^^std::string;
    }
    else if (value.find('.') != std::string_view::npos) {
        type = ^^double;
    }
    else {
        if (value == "true" || value == "false")
            type = ^^bool;
        else
            type = ^^int;
    }
    return std::meta::data_member_spec(type, {.name = std::string(name)});
}

// 用法：static constexpr char cfg[] = { #embed "config.ini" };
//       consteval { modforge::config_generator<^^Config>(cfg); }
export template <std::meta::info dest_class, std::size_t N>
constexpr void config_generator(const char (&config_str)[N]) {
    std::vector<std::meta::info> specs;
    std::size_t line_start = 0;
    for (std::size_t i = 0; i <= N - 1; ++i) {
        if (i == N - 1 || config_str[i] == '\n') {
            std::string_view line(&config_str[line_start], i - line_start);
            if (!line.empty())
                specs.push_back(line_to_spec(line));
            line_start = i + 1;
        }
    }
    std::meta::define_aggregate(dest_class, specs);
}

/** @brief 解析配置文件为键值表（运行时） */
std::map<std::string, std::string> parse_config(std::string_view path) {
    std::ifstream file{std::string(path)};
    if (!file.is_open())
        throw format_runtime_error("Could not open file {}", path);

    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(file, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        // 去引号 + trim
        auto clean = [](std::string_view s) {
            auto b = s.find_first_not_of(" \t\r\"");
            if (b == std::string_view::npos) return std::string_view{};
            return s.substr(b, s.find_last_not_of(" \t\r\"") - b + 1);
        };
        std::string_view lv(line);
        kv[std::string(clean(lv.substr(0, eq)))] = std::string(clean(lv.substr(eq + 1)));
    }
    return kv;
}

template <typename T>
void load_from_map(T& obj, const std::map<std::string, std::string>& kv) {
    static constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));

    template for (constexpr auto member : members) {
        constexpr std::string_view name = std::meta::identifier_of(member);
        auto it = kv.find(std::string(name));
        if (it == kv.end()) continue;   // 配置里没有 → 保持默认值

        using MemberType = typename[:std::meta::type_of(member):];
        if constexpr (std::is_same_v<MemberType, int>)
            obj.[:member:] = std::stoi(it->second);
        else if constexpr (std::is_same_v<MemberType, bool>)
            obj.[:member:] = (it->second == "true");
        else if constexpr (std::is_same_v<MemberType, double>)
            obj.[:member:] = std::stod(it->second);
        else if constexpr (std::is_same_v<MemberType, std::string>)
            obj.[:member:] = it->second;
    }
}

export void config_load(auto& obj, std::string_view path) {
    if (!std::filesystem::exists(path))
        throw format_runtime_error("{} is not exist", path);

    auto kv = parse_config(path);
    load_from_map(obj, kv);
}

NAMESPACE_END
