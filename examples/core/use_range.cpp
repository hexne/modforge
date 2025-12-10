/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 17:14:36
********************************************************************************/


/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 15:34:17
********************************************************************************/

import std;
import modforge.range;

void test_range() {

    // 使用数字圈定范围
    for (auto val : Range(1, 5))
        std::print("{} ", val);
    std::println("");

    // 使用数组圈定范围
    int arr[] = {1, 2, 3, 4, 5};
    for (auto val : Range(std::begin(arr), std::end(arr)))
        std::print("{} ", val);
    std::println("");

    // 使用数组圈定范围
    for (auto val : Range(arr))
        std::print("{} ", val);
    std::println("");

    // 使用容器圈定范围
    std::vector vec = {1, 2, 3, 4, 5};
    for (auto val : Range(vec))
        std::print("{} ", val);
    std::println("");

}