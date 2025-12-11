/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 15:34:17
********************************************************************************/

import std;
import modforge.range;

int test_range() {
    int flag{};

    // 使用数字圈定范围
    auto range1 = Range(1, 5);
    if (range1.distance() != 4)
        return __LINE__;
    flag = 1;
    for (auto val : range1) {
        if (flag ++ != val)
            return __LINE__;
    }

    // 使用数组圈定范围
    int arr[] = {1, 2, 3, 4, 5};
    auto range2 = Range(std::begin(arr), std::end(arr));
    if (range2.distance() != 5)
        return __LINE__;

    flag = 1;
    for (auto val : range2) {
        if (flag ++ != val)
            return __LINE__;
    }

    // 使用数组圈定范围
    auto range3 = Range(arr);
    if (range3.distance() != 5)
        return __LINE__;

    flag = 1;
    for (auto val : range3) {
        if (flag ++ != val)
            return __LINE__;
    }

    // 使用容器圈定范围
    std::vector vec = {1, 2, 3, 4, 5};
    auto range4 = Range(vec);
    if (range4.distance() != 5)
        return __LINE__;

    flag = 1;
    for (auto val : range4) {
        if (flag ++ != val)
            return __LINE__;
    }

    return 0;
}