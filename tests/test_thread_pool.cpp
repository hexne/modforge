import std;
import modforge.thread_pool;

int test_thread_pool() {
    modforge::ThreadPool pool(2, 8);

    auto sum = pool.submit([](int a, int b) { return a + b; }, 10, 32);
    auto square = pool.submit([](int v) { return v * v; }, 7);

    if (sum.get() != 42) return 1;
    if (square.get() != 49) return 2;

    std::vector<std::future<int>> tasks;
    tasks.reserve(8);
    for (int i = 0; i < 8; ++i) {
        tasks.emplace_back(pool.submit([i](int base) {
            return base + i;
        }, 100));
    }

    for (int i = 0; i < 8; ++i) {
        if (tasks[i].get() != 100 + i) return 3 + i;
    }

    return 0;
}
