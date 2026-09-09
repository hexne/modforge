import std;
import modforge.range;

int test_range() {
    using modforge::Range;

    // 整数区间：Range(begin, end)
    {
        int sum{}, count{};
        for (auto val : Range(1, 5)) {
            sum += val;
            ++count;
        }
        if (count != 4) return 1;
        if (sum != 1 + 2 + 3 + 4) return 2;

        Range r(1, 5);
        if (r.distance() != 4) return 3;
    }

    // 整数区间：Range(count) 等价 Range(0, count)
    {
        std::vector<int> got;
        for (auto val : Range(3))
            got.push_back(val);
        if (got.size() != 3) return 4;
        if (got[0] != 0 || got[1] != 1 || got[2] != 2) return 5;
    }

    // 整数区间：负数与空区间
    {
        int sum{}, count{};
        for (auto val : Range(-2, 2)) {
            sum += val;
            ++count;
        }
        if (count != 4) return 6;
        if (sum != -2) return 7;

        count = 0;
        for (auto val : Range(0, 0)) {
            (void)val;
            ++count;
        }
        if (count != 0) return 8;

        Range empty(0, 0);
        if (empty.distance() != 0) return 9;
    }

    // 指针区间
    int arr[] = {1, 2, 3, 4, 5};
    {
        int sum{}, count{};
        Range r(std::begin(arr), std::end(arr));
        for (auto val : r) {
            sum += val;
            ++count;
        }
        if (count != 5) return 10;
        if (sum != 15) return 11;
        if (r.distance() != 5) return 12;
    }

    // 数组区间
    {
        int sum{}, count{};
        Range r(arr);
        for (auto val : r) {
            sum += val;
            ++count;
        }
        if (count != 5) return 13;
        if (sum != 15) return 14;
        if (r.distance() != 5) return 15;
    }

    // 容器区间
    {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        int sum{}, count{};
        Range r(vec);
        for (auto val : r) {
            sum += val;
            ++count;
        }
        if (count != 5) return 16;
        if (sum != 15) return 17;
        if (r.distance() != 5) return 18;
    }

    // 容器区间：通过迭代器写回原容器
    {
        std::vector<int> vec = {1, 2, 3};
        for (auto &val : Range(vec))
            val *= 10;
        if (vec[0] != 10 || vec[1] != 20 || vec[2] != 30) return 19;
    }

    // 容器区间：空容器
    {
        std::vector<int> vec;
        int count{};
        for (auto val : Range(vec)) {
            (void)val;
            ++count;
        }
        if (count != 0) return 20;
    }

    // 正则匹配区间：底层迭代器按文本类型推导（编译期断言）
    {
        const std::string text = "a1 b2";
        std::string_view view = text;
        std::regex re("[a-z][0-9]+");

        // std::string 的迭代器是 string::const_iterator → sregex_iterator（解引用得 smatch）
        static_assert(std::same_as<decltype(Range(text, re)), Range<std::sregex_iterator>>);
        // string_view / 字符数组 / C 字符串的迭代器是 const char* → cregex_iterator（cmatch）
        static_assert(std::same_as<decltype(Range(view, re)), Range<std::cregex_iterator>>);
        static_assert(std::same_as<decltype(Range("a1 b2", re)), Range<std::cregex_iterator>>);
        static_assert(std::same_as<decltype(Range(text.c_str(), re)), Range<std::cregex_iterator>>);
        // 显式字符区间：按迭代器类型分派，与文本类型无关
        static_assert(std::same_as<decltype(Range(text.begin(), text.end(), re)),
                                   Range<std::sregex_iterator>>);
        static_assert(std::same_as<decltype(Range(text.data(), text.data() + text.size(), re)),
                                   Range<std::cregex_iterator>>);
        // 匹配迭代器对：正则特化结构上比通用迭代器对特化更特化，不会被后者抢走
        static_assert(std::same_as<decltype(Range(std::sregex_iterator{}, std::sregex_iterator{})),
                                   Range<std::sregex_iterator>>);
        static_assert(std::same_as<decltype(Range(std::cregex_iterator{}, std::cregex_iterator{})),
                                   Range<std::cregex_iterator>>);
    }

    // 正则匹配区间：Range(文本, 正则)
    {
        std::string text = "a1 b22 c333";
        std::regex re("[a-z][0-9]+");

        std::vector<std::string> got;
        // operator* 返回迭代器内部缓存的引用，++ 后即失效，故必须在循环内当场取出
        for (const auto &m : Range(text, re))
            got.push_back(m.str());
        if (got.size() != 3) return 21;
        if (got[0] != "a1" || got[1] != "b22" || got[2] != "c333") return 22;

        Range r(text, re);
        if (r.distance() != 3) return 23;
    }

    // 正则匹配区间：可重复遍历（begin() 按值返回，不推进成员）
    {
        std::string text = "x1 x2";
        std::regex re("x[0-9]");
        Range r(text, re);
        int first{}, second{};
        for (const auto &m : r) {
            (void)m;
            ++first;
        }
        for (const auto &m : r) {
            (void)m;
            ++second;
        }
        if (first != 2) return 24;
        if (second != 2) return 25;
    }

    // 正则匹配区间：由迭代器对构造
    {
        std::string text = "2026-09-09";
        std::regex re("[0-9]+");
        std::vector<std::string> got;
        for (const auto &m : Range(std::cregex_iterator(text.data(), text.data() + text.size(), re),
                                   std::cregex_iterator{}))
            got.push_back(m.str());
        if (got.size() != 3) return 26;
        if (got[0] != "2026" || got[1] != "09" || got[2] != "09") return 27;
    }

    // 正则匹配区间：无匹配即空区间
    {
        std::string text = "abc";
        std::regex re("[0-9]+");
        int count{};
        for (const auto &m : Range(text, re)) {
            (void)m;
            ++count;
        }
        if (count != 0) return 28;

        Range r(text, re);
        if (r.distance() != 0) return 29;
    }

    // 迭代器对区间：vector（random_access，distance 为 O(1)）
    {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        int sum{}, count{};
        Range r(vec.begin(), vec.end());
        for (auto val : r) {
            sum += val;
            ++count;
        }
        if (count != 5) return 32;
        if (sum != 15) return 33;
        if (r.distance() != 5) return 34;
    }

    // 迭代器对区间：list（bidirectional，distance 为 O(n)）
    {
        std::list<int> lst = {1, 2, 3};
        int sum{}, count{};
        Range r(lst.begin(), lst.end());
        for (auto val : r) {
            sum += val;
            ++count;
        }
        if (count != 3) return 35;
        if (sum != 6) return 36;
        if (r.distance() != 3) return 37;
    }

    // 迭代器对区间：经迭代器写回原容器，且可重复遍历
    {
        std::vector<int> vec = {1, 2, 3};
        Range r(vec.begin(), vec.end());
        for (auto &val : r)
            val *= 10;
        if (vec[0] != 10 || vec[1] != 20 || vec[2] != 30) return 38;

        int second{};
        for (auto val : r) {
            (void)val;
            ++second;
        }
        if (second != 3) return 39;
    }

    // 迭代器对区间：空区间
    {
        std::vector<int> vec;
        int count{};
        Range r(vec.begin(), vec.end());
        for (auto val : r) {
            (void)val;
            ++count;
        }
        if (count != 0) return 40;
        if (r.distance() != 0) return 41;
    }

    // 迭代器对区间：string 的迭代器
    {
        std::string text = "abc";
        std::string got;
        for (auto ch : Range(text.begin(), text.end()))
            got.push_back(ch);
        if (got != "abc") return 42;
    }

    // 正则匹配区间：string_view（与 string 共用同一特化，底层统一为 const char*）
    {
        std::string text = "a1 b22 c333";
        std::regex re("[a-z][0-9]+");
        std::vector<std::string> got;
        for (const auto &m : Range(std::string_view{text}, re))
            got.push_back(m.str());
        if (got.size() != 3) return 43;
        if (got[0] != "a1" || got[2] != "c333") return 44;

        // 只取子串：string_view 真正发挥作用的场景（"b22 "，不含后面的 c333）
        std::string_view sub{text.data() + 3, 4};
        std::vector<std::string> sub_got;
        for (const auto &m : Range(sub, re))
            sub_got.push_back(m.str());
        if (sub_got.size() != 1) return 45;
        if (sub_got[0] != "b22") return 46;
    }

    // 正则匹配区间：字符串字面量（字符数组，末位 '\0' 不参与匹配）
    {
        std::regex re("[a-z][0-9]+");
        std::vector<std::string> got;
        for (const auto &m : Range("a1 b22", re))
            got.push_back(m.str());
        if (got.size() != 2) return 47;
        if (got[0] != "a1" || got[1] != "b22") return 48;
    }

    // 正则匹配区间：C 字符串（按 '\0' 判定结束）
    {
        const char *text = "x9 y8 z7";
        std::regex re("[a-z][0-9]");
        std::vector<std::string> got;
        for (const auto &m : Range(text, re))
            got.push_back(m.str());
        if (got.size() != 3) return 49;
        if (got[0] != "x9" || got[2] != "z7") return 50;
    }

    // 正则匹配区间：由 [首指针, 尾指针) 构造，末尾可不为 '\0'
    {
        char buf[] = {'p', '1', 'q', '2'};
        std::regex re("[a-z][0-9]");
        std::vector<std::string> got;
        for (const auto &m : Range(buf, buf + sizeof(buf), re))
            got.push_back(m.str());
        if (got.size() != 2) return 51;
        if (got[0] != "p1" || got[1] != "q2") return 52;
    }

    // 正则匹配区间：捕获组
    {
        std::string text = "k=1, k=2";
        std::regex re("k=([0-9])");
        std::vector<std::string> got;
        for (const auto &m : Range(text, re))
            got.push_back(m[1].str());
        if (got.size() != 2) return 30;
        if (got[0] != "1" || got[1] != "2") return 31;
    }

    return 0;
}
