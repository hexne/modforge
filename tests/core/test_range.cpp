/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 15:34:17
********************************************************************************/

import std;
import modforge.range;

int test_range() {
    int flag{};

    // 使用数字圈定范围
    flag = 1;
    for (auto val : Range(1, 5)) {
        if (flag ++ != val)
            return __LINE__;
    }

    // 使用数组圈定范围
    int arr[] = {1, 2, 3, 4, 5};
    flag = 1;
    for (auto val : Range(std::begin(arr), std::end(arr))) {
        if (flag ++ != val)
            return __LINE__;
    }

    // 使用数组圈定范围
    flag = 1;
    for (auto val : Range(arr)) {
        if (flag ++ != val)
            return __LINE__;
    }

    // 使用容器圈定范围
    std::vector vec = {1, 2, 3, 4, 5};
    flag = 1;
    for (auto val : Range(vec)) {
        if (flag ++ != val)
            return __LINE__;
    }

    return 0;
}