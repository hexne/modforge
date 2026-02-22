/********************************************************************************
* @Author : hexne
* @Date   : 2026/02/21 00:55:38
********************************************************************************/
#include <meta>
import std;
import modforge.static_serialize;


struct Test {
    int i;
    double f;

    struct Sub {
        int sub_i;
        double sub_f;
    };
    void serialize(std::fstream &stream) {
        std::println("{}", "serialize(Test)");
    }
};


//
int test_static_serialize() {

    std::fstream file("serialize_test");
    int test_i = 10;
    modforge::serialize(test_i, file);

    Test val;
    modforge::serialize(val, file);
    modforge::deserialize(val, file);

    // serialize(&val); // 测试对指针的静态断言



    return 0;
}
