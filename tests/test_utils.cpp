import std;
import modforge;

int test_utils() {
    auto err = modforge::format_runtime_error("bad {} {}", "value", 123);
    if (std::string(err.what()).find("bad value 123") == std::string::npos) return 1;

    auto parts = modforge::StringUtils::split("a;b;c", ';');
    if (parts.size() != 3) return 2;
    if (parts[0] != "a" || parts[1] != "b" || parts[2] != "c") return 3;

    auto mixed = modforge::StringUtils::split("one,two;three", ';');
    if (mixed.size() != 2 || mixed[0] != "one,two" || mixed[1] != "three") return 4;

    return 0;
}
