import std;
import modforge.string_utils;

int test_string_utils() {
    auto parts = modforge::StringUtils::split("a,b,c", ',');
    if (parts.size() != 3) return 1;
    if (parts[0] != "a" || parts[1] != "b" || parts[2] != "c") return 2;

    auto mixed = modforge::StringUtils::split("alpha;beta;gamma", ';');
    if (mixed.size() != 3) return 3;
    if (mixed[0] != "alpha" || mixed[1] != "beta" || mixed[2] != "gamma") return 4;

    auto joined = modforge::StringUtils::join('-', "alpha", "beta", "gamma");
    if (joined != "alpha-beta-gamma") return 5;

    auto joined_str = modforge::StringUtils::join('/', std::string("x"), std::string("y"), std::string("z"));
    if (joined_str != "x/y/z") return 6;

    auto empty = modforge::StringUtils::split("", ',');
    if (!empty.empty()) return 7;

    auto single = modforge::StringUtils::split("solo", ',');
    if (single.size() != 1 || single[0] != "solo") return 8;

    return 0;
}
