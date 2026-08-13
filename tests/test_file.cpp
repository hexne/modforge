import std;
import file;

int test_file() {
    const auto root = std::filesystem::temp_directory_path() / "modforge_tests_file";
    std::filesystem::create_directories(root);
    const auto path = root / "sample.txt";
    const std::string content = "hello\nworld";

    modforge::File::write_file(path, content);
    auto read_back = modforge::File::read_file(path);
    if (read_back != content) return 1;

    const auto missing = root / "missing.txt";
    try {
        (void)modforge::File::read_file(missing);
        return 2;
    } catch (const std::runtime_error&) {
    }

    std::filesystem::remove_all(root);
    return 0;
}
