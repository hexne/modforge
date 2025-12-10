/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/06 17:01:59
********************************************************************************/
import modforge.average_queue;

int test_average_queue() {
    AverageQueue<int, 3> aq;
    // 初始化应该均值为0
    if (aq.average() != 0 or aq.size() != 0)
        return __LINE__;

    // 没有填满的时候有几个元素均值为几
    aq.push(10);
    if (aq.average() != 10 and aq.size() != 1)
        return __LINE__;

    aq.push(20);
    if (aq.average() != 15 and aq.size() != 2)
        return __LINE__;

    aq.push(30);
    if (aq.average() != 20 and aq.size() != 3)
        return __LINE__;

    // 应当挤掉第一个元素
    aq.push(40);
    if (aq.average() != 30 and aq.size() != 3)
        return __LINE__;

    AverageQueue<int, 2> aq2 {1, 2};
    if (aq2.size() != 2 or aq2.average() != 1)
        return __LINE__;

    // AverageQueue<int, 2> aq3 {1, 2, 3};

    return 0;
}