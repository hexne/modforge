import std;
import modforge;

int test_timer() {
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

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        if (fired.load() > 0 && repeat_fired.load() >= 2) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (fired.load() != 1) return 1;
    if (repeat_fired.load() < 2) return 2;

    auto cancelled_id = timer.add_task([&] {
        canceled.fetch_add(1);
    }, std::chrono::milliseconds(60));
    timer.remove(cancelled_id);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (canceled.load() != 0) return 3;

    timer.remove(once_id);
    timer.remove(repeat_id);
    return 0;
}
