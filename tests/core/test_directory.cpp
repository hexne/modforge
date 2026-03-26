/********************************************************************************
* @Author : hexne
* @Date   : 2026/01/16 15:40:52
********************************************************************************/

import modforge.directory;
import std;

int test_directory() {
    namespace fs = std::filesystem;
    using modforge::Directory;

    // 准备测试目录
    auto test_root = fs::temp_directory_path() / std::format(
        "modforge_directory_test_{}",
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    fs::remove_all(test_root);
    fs::create_directories(test_root / "sub");
    auto fail = [&](int line) {
        fs::remove_all(test_root);
        return line;
    };

    // 创建测试文件：
    // a.text / b.docx / noext / sub/c.text
    auto write_file = [](const fs::path& file) {
        std::ofstream out(file);
    };

    write_file(test_root / "a.text");
    write_file(test_root / "b.docx");
    write_file(test_root / "noext");
    write_file(test_root / "sub" / "c.text");

    Directory directory(test_root.string(), true);
    auto sub_text = (fs::path("sub") / "c.text").string();
    auto has = [](const std::vector<std::string>& files, const std::string& name) {
        return std::find(files.begin(), files.end(), name) != files.end();
    };

    // extension == ""：不过滤，应返回全部 4 个文件
    auto all = directory.files();
    if (all.size() != 4)
        return fail(__LINE__);

    // extension == "text"：只返回 text 后缀文件
    auto text = directory.files("text");
    if (text.size() != 2
        || !has(text, "a.text")
        || !has(text, sub_text))
        return fail(__LINE__);

    // extension == ";text"：返回无后缀 + text 后缀
    auto text_or_noext = directory.files(";text");
    if (text_or_noext.size() != 3
        || !has(text_or_noext, "a.text")
        || !has(text_or_noext, sub_text)
        || !has(text_or_noext, "noext"))
        return fail(__LINE__);

    // 清理测试目录
    fs::remove_all(test_root);
    return 0;
}
