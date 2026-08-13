import std;
import modforge;

int test_args_parser() {
    std::vector<std::string> args = {
        "app",
        "--verbose",
        "name=value",
        "mode=debug",
        "--dry-run"
    };

    ArgsParser parser(args);
    if (!parser.has_flag("--verbose")) return 1;
    if (!parser.has_flag("--dry-run")) return 2;
    if (parser.has_flag("name=value")) return 3;

    if (parser.get_value("name") != "value") return 4;
    if (parser.get_value("mode") != "debug") return 5;
    if (parser.get_value("missing", "fallback") != "fallback") return 6;

    auto now = modforge::Time::now();
    if (now.get_string().empty()) return 7;

    return 0;
}
