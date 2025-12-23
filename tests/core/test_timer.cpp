/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/23 17:01:05
********************************************************************************/

import modforge.timer;
import std;

int test_timer() {
    Timer timer;

    timer.add_repeat_task([]{
        std::println("任务1");
    }, std::chrono::seconds{1});

    timer.add_repeat_task([]{
        std::println("任务2");
    }, std::chrono::seconds{2});

    timer.add_task([]{
        std::println("任务3");
    }, std::chrono::seconds{2});

    while (!timer.is_finish())
        ;

    return 0;
}