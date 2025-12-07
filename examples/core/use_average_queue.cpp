/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/06 18:09:15
********************************************************************************/
import modforge.average_queue;


void use_average_queue() {
    // AverageQueue的模板参数T 需要支持 operator + (T), operator / (int), T {} 三个操作
    // 默认初始化
    AverageQueue<int, 3> aq1;

    // 列表初始化
    AverageQueue<int, 3> aq2 {1, 2, 3};

    // 向循环队列中添加元素
    aq1.push(1);

    // 获取循环队列中有效元素数量
    auto tmp = aq1.size();

    // 获取循环队列中有效元素均值
    auto average = aq1.average();

}