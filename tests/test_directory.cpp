import std;
import modforge;

int test_directory() {
    const auto root = std::filesystem::temp_directory_path() / "modforge_tests_directory";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "nested" / "deep");

    modforge::File::write_file(root / "alpha.txt", "alpha");
    modforge::File::write_file(root / "beta.md", "beta");
    modforge::File::write_file(root / "nested" / "gamma.txt", "gamma");
    modforge::File::write_file(root / "nested" / "deep" / "delta.txt", "delta");

    modforge::Directory dir(root);
    auto txt_files = dir.files(".txt");
    if (txt_files.size() != 3) return 1;

    auto txt_names = dir.files(".txt", true);
    if (std::ranges::count(txt_names, std::string("alpha.txt")) != 1) return 2;
    if (std::ranges::count(txt_names, std::string("gamma.txt")) != 1) return 3;
    if (std::ranges::count(txt_names, std::string("delta.txt")) != 1) return 4;

    modforge::Directory shallow(root, 2);
    auto shallow_txt_files = shallow.files(".txt", false, true);
    if (shallow_txt_files.size() != 2) return 5;

    std::filesystem::remove_all(root);
    return 0;
}
