import std;
import modforge.config_generator;

// 场景 1：编译期生成类型（config_generator）——前向声明，由 config_generator 补全定义
struct GenConfig;

static constexpr char gen_cfg[] =
    "name=\"xiaoming\"\n"
    "age=20\n"
    "ratio=0.5\n"
    "enabled=true\n";

consteval {
    modforge::config_generator<^^GenConfig>(gen_cfg);
}

// 场景 2：运行时加载（config_load）——需完整类型，故用独立类型
struct LoadConfig {
    std::string name;
    int age;
    double ratio;
    bool enabled;
};

int test_config_generator() {
    // --- 场景 1：编译期生成的类型，成员存在且类型推断正确 ---
    GenConfig g{};
    static_assert(std::is_same_v<decltype(g.name), std::string>);
    static_assert(std::is_same_v<decltype(g.age), int>);
    static_assert(std::is_same_v<decltype(g.ratio), double>);
    static_assert(std::is_same_v<decltype(g.enabled), bool>);

    g.name = "xiaoming";
    g.age = 20;
    g.ratio = 0.5;
    g.enabled = true;
    if (g.name != "xiaoming" || g.age != 20 || g.ratio != 0.5 || !g.enabled)
        return 1;   // 编译期生成或类型推断失败

    // --- 场景 2：运行时从文件加载并填充（string/int/double/bool 全类型）---
    auto path = std::filesystem::temp_directory_path() / "modforge_test_config_generator.ini";
    {
        std::ofstream out{path};
        out << "name=\"x\"\nage=30\nratio=1.5\nenabled=false\n";
    }

    LoadConfig lc{};
    bool threw = false;
    try {
        modforge::config_load(lc, path.native());
    } catch (...) {
        threw = true;
    }
    std::filesystem::remove(path);

    if (threw) return 2;                                   // 加载过程抛异常
    if (lc.name != "x" || lc.age != 30) return 3;          // string/int 加载错误
    if (lc.ratio != 1.5 || lc.enabled != false) return 4;  // double/bool 加载错误

    // --- 场景 3：文件不存在应抛异常 ---
    threw = false;
    try {
        modforge::config_load(lc, (std::filesystem::temp_directory_path() / "modforge_no_such_file.ini").native());
    } catch (...) {
        threw = true;
    }
    if (!threw) return 5;   // 文件不存在却未抛异常

    return 0;
}
