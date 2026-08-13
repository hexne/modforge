import std;
import modforge;

int test_modforge() {
    ArgsParser parser({"app", "--debug", "user=alice"});
    if (!parser.has_flag("--debug")) return 1;
    if (parser.get_value("user") != "alice") return 2;

    modforge::Time timestamp(2024, 1, 2, 3, 4, 5);
    if (timestamp.get<std::chrono::year>() != 2024) return 3;
    if (timestamp.get<std::chrono::month>() != 1) return 4;
    if (timestamp.get<std::chrono::day>() != 2) return 5;

    return 0;
}
