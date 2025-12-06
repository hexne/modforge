/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/06 17:01:59
********************************************************************************/
import modforge.average_queue;

int test_average_queue() {
    AverageQueue<int, 3> aq;
    // 初始化应该均值为0
    if (aq.average() != 0)
        return __LINE__;

    // 没有填满的时候有几个元素均值为几
    aq.push(10);
    aq.push(20);
    if (aq.average() != 15)
        return __LINE__;

    aq.push(30);
    if (aq.average() != 20)
        return __LINE__;

    // 挤掉第一个元素
    aq.push(40);
    if (aq.average() != 30)
        return __LINE__;


    return 0;
}