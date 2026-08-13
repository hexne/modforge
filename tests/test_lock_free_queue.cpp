import std;
import modforge.lock_free_queue;

int test_lock_free_queue() {
    SPMCQueue<int> q(2);
    if (!q.push(10)) return 1;
    if (!q.push(20)) return 2;
    if (q.push(30)) return 3;

    auto first = q.try_pop();
    if (!first || *first != 10) return 4;

    auto second = q.try_pop();
    if (!second || *second != 20) return 5;

    if (q.try_pop().has_value()) return 6;

    SPMCQueue<std::string> strings(1);
    if (!strings.push(std::string("abc"))) return 7;
    auto item = strings.try_pop();
    if (!item || *item != std::string("abc")) return 8;

    return 0;
}
