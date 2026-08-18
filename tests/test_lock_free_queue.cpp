import std;
import modforge.lock_free_queue;

int test_lock_free_queue() {
    // SPSCQueue: bounded, FIFO, full/empty checks
    modforge::SPSCQueue<int> spsc(3);
    if (!spsc.push(10)) return 1;
    if (!spsc.push(20)) return 2;
    if (spsc.empty()) return 3;
    if (spsc.size() != 2) return 4;

    auto spsc_first = spsc.pop();
    if (!spsc_first || *spsc_first != 10) return 5;
    if (spsc.size() != 1) return 6;

    spsc.clear();
    if (!spsc.empty()) return 7;

    // SPSCQueue with string payload
    modforge::SPSCQueue<std::string> spsc_strings(2);
    if (!spsc_strings.push(std::string("hello"))) return 8;
    if (!spsc_strings.push(std::string("world"))) return 9;
    auto str_first = spsc_strings.pop();
    if (!str_first || *str_first != std::string("hello")) return 10;
    auto str_second = spsc_strings.pop();
    if (!str_second || *str_second != std::string("world")) return 11;
    if (spsc_strings.pop().has_value()) return 12;

    // MPSCQueue: producer-consumer single-head semantics
    modforge::MPSCQueue<int> mpsc;
    mpsc.push(1);
    mpsc.push(2);
    auto mpsc_first = mpsc.pop();
    if (!mpsc_first || *mpsc_first != 1) return 13;
    auto mpsc_second = mpsc.pop();
    if (!mpsc_second || *mpsc_second != 2) return 14;
    if (mpsc.pop().has_value()) return 15;

    // SPMCQueue: bounded ring with push failure when full
    modforge::SPMCQueue<int> spmc(2);
    if (!spmc.push(10)) return 16;
    if (!spmc.push(20)) return 17;
    if (spmc.push(30)) return 18;

    auto spmc_first = spmc.pop();
    if (!spmc_first || *spmc_first != 10) return 19;
    auto spmc_second = spmc.pop();
    if (!spmc_second || *spmc_second != 20) return 20;
    if (spmc.pop().has_value()) return 21;

    // MPMCQueue: bounded ring, FIFO behavior, and empty check
    modforge::MPMCQueue<int> mpmc(2);
    if (!mpmc.push(77)) return 22;
    if (!mpmc.push(88)) return 23;
    if (mpmc.push(99)) return 24;

    auto mpmc_first = mpmc.pop();
    if (!mpmc_first || *mpmc_first != 77) return 25;
    auto mpmc_second = mpmc.pop();
    if (!mpmc_second || *mpmc_second != 88) return 26;
    if (mpmc.pop().has_value()) return 27;

    return 0;
}
