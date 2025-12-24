/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/23 17:01:05
********************************************************************************/

import modforge.timer;
import std;

int test_timer() {
    Timer timer;
    int count1{};
    int count2{};
    int count3{};

    int id1 = timer.add_task([&]{
        count1 ++;
    }, std::chrono::seconds{1}, 5);

    int id2 = timer.add_task([&]{
        count2 ++;
    }, std::chrono::seconds{2}, 3);

    int id3 = timer.add_task([&]{
        count3 ++;
    }, std::chrono::seconds{2});

    timer.remove(id1);

    while (timer.task_count())
        ;
    if (count1 != 0 or count2 != 3 or count3 != 1) {
        return __LINE__;
    }

    return 0;
}