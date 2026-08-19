/********************************************************************************
* @Author : hexne
* @Date   : 2026/08/19 23:05:27
********************************************************************************/
import modforge;
import std;

struct Struct {
    [[=modforge::serialize_flag::ignore]] int x;
    [[=modforge::serialize_flag::id{10}]] int y;
    struct Z {
        // [[=modforge::serialize_flag::id{20}]]
        float val;
    } z;
    std::vector<int> numbers;
    std::string string;

    bool operator == (const Struct& other) const {
        if (y != other.y) return false;
        // if (x != other.x) return false;
        if (z.val != other.z.val) return false;

        if (numbers.size() != other.numbers.size()) return false;

        for (int i = 0; i < numbers.size(); i++)
            if (numbers[i] != other.numbers[i]) return false;
        if (string != other.string) return false;

        return true;
    }
};

int test_static_serialize() {
    unsigned char buf[512]{};
    // Struct s {.x = 10, .y = 20, .z = {.val = 30} }, s2;
    Struct s {.x = 10, .y = 20, .z = {.val = 30},
        .numbers = { 1, 2, 3, 4, 5 },
        .string = "hello"}, s2;
    modforge::ArchivePointer ar(buf);
    modforge::serialize(s, ar);

    modforge::ArchivePointer ar2(buf);
    modforge::deserialize(s2, ar2);

    if (s == s2) {
        std::println("==");
    }
    else {
        std::println("!=");
    }

    if (s == s2)
        return 0;
    return -1;
}