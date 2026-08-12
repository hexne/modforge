import std;
import modforge.time;

int test_time() {
    using namespace std::chrono;

    auto t = modforge::UTCTime<milliseconds>(2024, 1, 2, 3, 4, 5);
    if (t.get<year>() != 2024) return 1;
    if (t.get<month>() != 1) return 2;
    if (t.get<day>() != 2) return 3;
    if (t.get<hours>() != 3) return 4;
    if (t.get<minutes>() != 4) return 5;
    if (t.get<seconds>() != 5) return 6;

    auto parsed = modforge::UTCTime<milliseconds>::from_string("2024-01-02 03:04:05");
    if (!(parsed == t)) return 7;

    auto slash_parsed = modforge::UTCTime<milliseconds>::from_string("2024/02/03 04:05:06");
    if (slash_parsed.get<year>() != 2024 || slash_parsed.get<month>() != 2 || slash_parsed.get<day>() != 3) return 8;

    auto shifted = t + seconds(2);
    if (!(shifted > t)) return 9;

    auto text = t.get_string();
    if (text.find("2024") == std::string::npos) return 10;

    try {
        (void)modforge::UTCTime<milliseconds>::from_string("not-a-time");
        return 11;
    } catch (const std::exception&) {
    }

    return 0;
}
