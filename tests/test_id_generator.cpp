import std;
import modforge;

// IDGenerator 测试：原子递增 id 生成器。
// 语义：next() 从 1 开始，返回当前值并递增（1, 2, 3, ...）。
int test_id_generator() {
    // 1) 顺序递增
    {
        modforge::IDGenerator gen;
        if (gen.next() != 1) return 1;
        if (gen.next() != 2) return 2;
        if (gen.next() != 3) return 3;
    }

    // 2) 实例相互独立：每个生成器各自从 1 开始
    {
        modforge::IDGenerator a, b;
        if (a.next() != 1) return 4;
        if (b.next() != 1) return 5;
        if (a.next() != 2) return 6;
        if (b.next() != 2) return 7;
    }

    // 3) 并发唯一性：8 线程 × 1000 次 = 8000 个 id，
    //    必须全部互不相同且连续无空洞（范围恰为 [1, 8000]）。
    //    这是 atomic 计数器的核心卖点：并发下不重复、不丢号。
    {
        modforge::IDGenerator gen;
        constexpr int kThreads = 8;
        constexpr int kPerThread = 1000;
        constexpr int kTotal = kThreads * kPerThread;

        std::vector<std::thread> threads;
        std::vector<std::vector<int>> collected(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&gen, &collected, t] {
                collected[t].reserve(kPerThread);
                for (int i = 0; i < kPerThread; ++i)
                    collected[t].push_back(gen.next());
            });
        }
        for (auto& th : threads)
            th.join();

        std::vector<int> all;
        for (const auto& v : collected)
            all.insert(all.end(), v.begin(), v.end());

        // 互不相同
        std::sort(all.begin(), all.end());
        if (std::adjacent_find(all.begin(), all.end()) != all.end())
            return 8;

        // 连续无空洞（最小值 1，最大值 kTotal，且数量恰为 kTotal）
        if (all.front() != 1) return 9;
        if (all.back() != kTotal) return 10;
        if (all.size() != static_cast<std::size_t>(kTotal)) return 11;
    }

    return 0;
}
