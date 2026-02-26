/********************************************************************************
* @Author : hexne
* @Date   : 2026/02/21 00:55:38
********************************************************************************/
#include <meta>
import std;
import modforge.static_serialize;


struct Base {
    struct Sub {
        int i{};
    }s;
    int base{};
    Base() = default;
    Base(int s, int b) : s(s), base(b) {  }

    bool operator==(const Base& b) const {
        return s.i == b.s.i and base==b.base;
    }
};

class Base2 {
    int val{};
    std::vector<int> vec{};
    std::array<int, 5> arr{};
public:
    Base2() = default;
    Base2(const std::initializer_list<int> &list) : vec(list) {
        val = static_cast<int>(list.size());
        for (auto &num : arr)
            num = val;
    }
};


class Test : Base , public Base2 {
    int i{};
    double f{};
public:
    Test() = default;
    Test(int s, int b, int i, double f, const std::initializer_list<int> &list) : Base(s, b), i(i), f(f), Base2(list) {  }
    bool operator==(const Test& t) const {
        return t.i == i and t.f == f
            and Base::operator==(t);
    }
};


//
int test_static_serialize() {
    using namespace modforge;
    // Test test(1, 2, 3, 4.5f);
    // static constexpr auto bases = std::define_static_array(
    //     std::meta::bases_of(^^Test, std::meta::access_context::unchecked())
    // );
    // template for (constexpr auto info : bases) {
    //     auto &base = test.[: info :];
    //     std::println("{}", std::meta::display_string_of(^^decltype(base)));
    // }



    std::fstream out("serialize_test", std::ios::binary | std::ios::out);
    if (!out.is_open())
        return __LINE__;

    // 序列化测试
    int test_i = 10;
    serialize(test_i, out);

    Test val(1, 2, 3, 4.5f, {1, 2, 3, 4, 5});
    serialize(val, out);

    // serialize(&val); // 测试对指针的静态断言
    out.close();
    // 反序列化
    std::fstream in("serialize_test", std::ios::binary | std::ios::in);
    if (!in.is_open())
        return __LINE__;

    int test_i2{};
    deserialize(test_i2, in);
    if (test_i != test_i2)
        return __LINE__;

    Test val2;
    deserialize(val2, in);
    if (val != val2) {
        return __LINE__;
    }



    return 0;
}
