/********************************************************************************
* @Author : hexne
* @Date   : 2026/03/25 20:36:00
********************************************************************************/

import modforge.directory;
import std;

void use_directory() {
    using modforge::Directory;

    auto print_list = [](std::string_view title, std::vector<std::string> items) {
        std::ranges::sort(items);
        std::println("\n[{}] count={}", title, items.size());
        for (const auto& item : items)
            std::println("  {}", item);
    };

    // 相对路径 + 深度限制 1
    Directory rel_dir(".", true, 1);
    print_list("files()", rel_dir.files());

    // 扩展过滤
    print_list("files(\"text\")", rel_dir.files("text"));
    print_list("files(\"text;docx\")", rel_dir.files("text;docx"));
    print_list("files(\";text\")", rel_dir.files(";text"));
    print_list("files(\";;;\")", rel_dir.files(";;;"));

    // 路径/名称输出选项
    print_list("files(\"\", true, false)  // only_file", rel_dir.files("", true, false));
    print_list("files(\"\", false, true)  // only_name", rel_dir.files("", false, true));

    // 目录列表
    print_list("directorys(false)", rel_dir.directorys(false));
    print_list("directorys(true)", rel_dir.directorys(true));

    // 对比：绝对路径输出
    Directory abs_dir(".", false, 1);
    print_list("abs_dir.files()", abs_dir.files());

    // 对比：deep = 0 不限制深度
    Directory all_depth(".", true, 0);
    print_list("all_depth.files(\"text\")", all_depth.files("text"));
}
