import std;
import modforge;

modforge::TimerTask<int> waiter(modforge::CoroutineTimer &timer, std::chrono::milliseconds d) {
    co_await timer.sleep_for(d);
    co_return 123;
}

int test_timer() {
    // --- Coroutine-based Timer tests ---
    {
        modforge::CoroutineTimer timer;

        // 单个协程等待
        auto task = waiter(timer, std::chrono::milliseconds(20));

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (!task.done() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            timer.resume();
        }

        if (!task.done()) return 1;
        if (task.result() != 123) return 2;

        // 多个协程按顺序唤醒
        auto t1 = waiter(timer, std::chrono::milliseconds(10));
        auto t2 = waiter(timer, std::chrono::milliseconds(30));

        const auto deadline2 = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while ((!t1.done() || !t2.done()) && std::chrono::steady_clock::now() < deadline2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            timer.resume();
        }

        if (!t1.done() || !t2.done()) return 3;
        if (t1.result() != 123 || t2.result() != 123) return 4;

        // resume on empty should return nullopt and not crash
        timer.resume();
    }

    // --- Thread-based Timer tests ---
    {
        std::atomic_int fired{0};
        std::atomic_int repeat_fired{0};
        std::atomic_int canceled{0};

        modforge::Timer timer;
        auto once_id = timer.add_task([&] {
            fired.fetch_add(1);
        }, std::chrono::milliseconds(25));

        auto repeat_id = timer.add_task([&] {
            repeat_fired.fetch_add(1);
        }, std::chrono::milliseconds(15), 2);

        const auto deadline3 = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (std::chrono::steady_clock::now() < deadline3) {
            if (fired.load() > 0 && repeat_fired.load() >= 2) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        if (fired.load() != 1) return 5;
        if (repeat_fired.load() < 2) return 6;

        auto cancelled_id = timer.add_task([&] {
            canceled.fetch_add(1);
        }, std::chrono::milliseconds(60));
        timer.remove(cancelled_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        if (canceled.load() != 0) return 7;

        timer.remove(once_id);
        timer.remove(repeat_id);
    }

    return 0;
}
