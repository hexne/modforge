/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 15:33:24
*
*  从 modforge_back 的 core 模块迁移而来（game-server-dev 中的版本与之同源）。
*  迁移改动：模块名改为 modforge.range，整体包进 NAMESPACE，补齐接口注释。
********************************************************************************/

module;
export module modforge.range;
import std;

/** @brief 容器判别：具备 begin / end / size 成员的类型才算容器
 *  @tparam T 待判别类型
 *  @note 形参按值，故不可拷贝的类型会被判为非容器；本模板定义在全局命名空间
 */
template <typename T>
struct IsContainer : std::false_type { };

template <typename T>
    requires requires (T t) { t.begin(); t.end(); t.size(); }
struct IsContainer<T> : std::true_type { };


NAMESPACE_BEGIN

/** @brief 区间封装：把整数区间 / 指针区间 / 数组 / 容器统一成可 range-for 的对象
 *  @tparam T 区间类型，按 T 的性质分派到下面的四个特化
 *  @note 其迭代器只满足裸 range-for 的要求：没有 value_type / difference_type /
 *        iterator_category，没有后置 ++，begin / end 也没有 const 重载，
 *        因此不能用于 std::ranges 的算法
 */
export template <typename T>
class Range;


/** @brief 整数区间 [begin, end)，每次 ++ 步进 1
 *  @tparam T 整型
 *  @note distance() 由 end - begin 得到（有符号），begin > end 时转成 size_t 会回绕
 */
template <typename T>
    requires std::is_integral_v<T>
class Range<T>{
public:
    /** @brief 整数区间迭代器，持有当前值
     *  @note operator* 返回内部成员的引用，解引用得到的引用会随 ++ 变化
     */
    class iterator {
        T val_{};
    public:
        iterator() = default;
        explicit iterator(T val) : val_(val) {  }
        iterator& operator++() {
            ++ val_;
            return *this;
        }
        bool operator != (const iterator &other) {
            return val_ != other.val_;
        }
        T& operator *() {
            return val_;
        }
        T operator - (const iterator& other) const {
            return val_ - other.val_;
        }
    };
    /** @brief 构造 [0, count)
     *  @param count 区间上界（不含），恒按 int 传入
     */
    explicit Range(int count) : Range(0, count) {  }
    /** @brief 构造 [begin, end)
     *  @param begin 区间下界（含）
     *  @param end 区间上界（不含），恒按 int 传入
     */
    Range(int begin, int end) : begin_(iterator(begin)), end_(iterator(end)) {  }
    iterator begin() {
        return begin_;
    }
    iterator end() {
        return end_;
    }
    /** @brief 区间长度
     *  @return end - begin
     */
    [[nodiscard]] std::size_t distance() const {
        return end_ - begin_;
    }
private:
    iterator begin_, end_;
};

/** @brief 指针区间 [begin_pointer, end_pointer)
 *  @tparam T 指针类型
 */
template <typename T>
    requires std::is_pointer_v<T>
class Range<T> {
public:
    /** @brief 指针区间迭代器，持有当前指针 */
    class iterator {
        T point_{};
    public:
        iterator() = default;
        explicit iterator(T point) : point_(point) {  }
        iterator& operator++() {
            ++ point_;
            return *this;
        }
        bool operator != (const iterator &other) {
            return point_ != other.point_;
        }
        std::remove_pointer_t<T>& operator *() {
            return *point_;
        }
        int operator - (const iterator& other) {
            return point_ - other.point_;
        }
    };
    /** @brief 构造指针区间
     *  @param begin_pointer 起始指针（含）
     *  @param end_pointer 结束指针（不含）
     */
    Range(T begin_pointer, T end_pointer) : begin_(begin_pointer), end_(end_pointer) {  }
    iterator begin() {
        return begin_;
    }
    iterator end() {
        return end_;
    }
    /** @brief 区间长度
     *  @return end - begin（元素个数，按指针差值算）
     */
    std::size_t distance() {
        return end_ - begin_;
    }
private:
    iterator begin_, end_;
};

/** @brief 数组区间，覆盖整个数组
 *  @tparam T 数组类型（如 int[5]）
 */
template <typename T>
    requires std::is_array_v<T>
class Range<T> {
    using PT = std::decay_t<T>;
    using Iterator = typename Range<PT>::iterator;
    Iterator begin_{}, end_{};
public:
    /** @brief 构造数组区间
     *  @param arr 数组左值引用，其生命周期须长于本对象
     */
    explicit Range(T &arr) : begin_(std::begin(arr)), end_(std::end(arr)) { }
    Iterator begin() {
        return begin_;
    }
    Iterator end() {
        return end_;
    }
    /** @brief 区间长度
     *  @return 数组元素个数
     */
    std::size_t distance() {
        return end_ - begin_;
    }

};

/** @brief 容器区间，覆盖容器的全部元素
 *  @tparam T 容器类型（需具备 begin / end / size）
 *  @note 只接受非 const 左值；内部保存容器指针，容器先于本对象销毁会悬垂
 */
template <typename T>
    requires IsContainer<T>::value
class Range<T> {
    T *container_{};
public:
    /** @brief 容器区间迭代器，包装容器的 iterator */
    class iterator {
        typename T::iterator iterator_{};
    public:
        iterator() = default;
        explicit iterator(decltype(iterator_) iterator) : iterator_(iterator) { }

        iterator& operator++() {
            ++ iterator_;
            return *this;
        }
        bool operator != (const iterator &other) const {
            return iterator_ != other.iterator_;
        }
        int operator - (const iterator& other) const {
            return iterator_ - other.iterator_;
        }
        auto& operator *() {
            return *iterator_;
        }
    };

    /** @brief 构造容器区间
     *  @param container 容器左值引用，其生命周期须长于本对象
     */
    explicit Range(T &container) :
        container_(&container), begin_(container.begin()), end_(container.end()) {  }
    auto begin() {
        return begin_;
    }
    auto end() {
        return end_;
    }
    /** @brief 区间长度
     *  @return container.size()
     */
    std::size_t distance() {
        return container_->size();
    }
private:
    iterator begin_, end_;
};

/** @brief 迭代器对区间 [begin, end)，覆盖任意非指针迭代器
 *  @tparam T 任意 std::input_iterator；指针另由上面的指针特化承接，故在此排除
 *  @note begin() 按值返回故可重复遍历；distance() 的复杂度由 std::distance 按
 *        迭代器类别分派：random_access 为 O(1)，其余为 O(n)
 */
template <typename T>
    requires std::input_iterator<T> && (!std::is_pointer_v<T>)
class Range<T> {
public:
    using iterator = T;

    /** @brief 构造迭代器对区间
     *  @param begin 起始迭代器（含）
     *  @param end   结束迭代器（不含），所指序列的生命周期须长于本对象
     */
    Range(iterator begin, iterator end) : begin_(begin), end_(end) {  }

    iterator begin() const { return begin_; }
    iterator end() const { return end_; }

    /** @brief 区间长度
     *  @return 元素个数
     *  @note 需要 T 可拷贝；不满足时该函数不参与重载决议
     */
    [[nodiscard]] std::size_t distance() const requires std::copyable<T> {
        return static_cast<std::size_t>(std::distance(begin_, end_));
    }
private:
    iterator begin_, end_;
};

/** @brief 正则匹配区间：遍历文本中所有匹配
 *  @tparam It 底层字符迭代器，由文本类型反推：std::string 取
 *             string::const_iterator（即 std::sregex_iterator），
 *             string_view / 字符数组 / C 字符串取 const char*（即 std::cregex_iterator）
 *  @note 本特化按实参自动选定底层迭代器，故 std::string 得到 smatch、其余得到
 *        cmatch，无需按文本类型各写一个特化；同理不能反向混用——
 *        std::string 的迭代器造不出 cregex_iterator，反之亦然
 *  @note std::regex_iterator 是 input iterator：distance() 为 O(n)；operator*
 *        返回内部缓存的引用，++ 后先前取得的引用失效；文本与 regex 的生命周期
 *        都必须长于本对象
 */
template <typename It>
class Range<std::regex_iterator<It>> {
public:
    using iterator = std::regex_iterator<It>;

    /** @brief 由 [first, last) 字符迭代器对构造，其余文本构造最终都落到这里
     *  @param first 文本起始（含）
     *  @param last  文本结束（不含）
     *  @param re    正则对象（生命周期须长于本对象）
     *  @note It 原样取用，不做 const 化：非 const 容器的 begin() 会实例化出
     *        regex_iterator<T*> 版本，需要 std::smatch / std::cmatch 时用 const 迭代器
     */
    Range(It first, It last, const std::regex &re) : begin_(first, last, re) {  }

    /** @brief 由字符区间构造（std::string / std::string_view / 字符数组等）
     *  @param text 待匹配文本，其底层字符序列的生命周期须长于本对象
     *  @param re   正则对象（生命周期须长于本对象）
     *  @note 仅当 text 的迭代器类型与本特化的 It 一致时可用，
     *        这条约束把文本类型与 sregex / cregex 绑死，避免隐式转换
     */
    template <typename Text>
        requires requires (const Text &t) { { std::ranges::begin(t) } -> std::same_as<It>; }
    Range(const Text &text, const std::regex &re)
        : Range(std::ranges::begin(text), std::ranges::end(text), re) {  }

    /** @brief 由 C 字符串构造，按首个 '\0' 判定结束
     *  @param text 以 '\0' 结尾的字符序列，生命周期须长于本对象
     *  @param re   正则对象（生命周期须长于本对象）
     */
    Range(const char *text, const std::regex &re) requires std::same_as<It, const char *>
        : Range(text, text + std::string_view{text}.size(), re) {  }

    /** @brief 由字符数组构造（如字符串字面量）
     *  @param text 字符数组；末元素为 '\0' 时不参与匹配（按 C 字符串处理），
     *              故不以 '\0' 结尾的 char buf[] 也能正确取到末尾字符
     *  @param re   正则对象（生命周期须长于本对象）
     */
    template <std::size_t N>
        requires std::same_as<It, const char *>
    Range(const char (&text)[N], const std::regex &re)
        : Range(text, text + N - (text[N - 1] == '\0' ? 1 : 0), re) {  }

    /** @brief 由 [begin, end) 匹配迭代器对构造
     *  @param begin 首个匹配位置的迭代器
     *  @param end   结束迭代器（默认构造的 std::regex_iterator<It>）
     */
    Range(iterator begin, iterator end) : begin_(begin), end_(end) {  }

    iterator begin() const { return begin_; }   // 必须按值返回，否则二次遍历失效
    iterator end() const { return end_; }

    /** @brief 匹配个数
     *  @return 遍历计数的结果，复杂度 O(n)（与其余特化的 O(1) 不同）
     */
    [[nodiscard]] std::size_t distance() const {
        return static_cast<std::size_t>(std::distance(iterator{begin_}, end_));
    }
private:
    iterator begin_, end_{};
};


/** @brief 推导指引：单参数整数推成 Range<int> */
Range(int) -> Range<int>;

/** @brief 推导指引：两个同类型参数（整数或指针）推成 Range<T> */
template <typename T>
Range(T, T) -> Range<T>;

/** @brief 推导指引：单个左值（数组或容器）推成 Range<T> */
template <typename T>
Range(T &) -> Range<T>;

/** @brief 推导指引：字符区间 + 正则 按文本的迭代器类型选定底层迭代器
 *  @note std::string 的迭代器是 string::const_iterator → std::sregex_iterator（smatch）；
 *        string_view / 字符数组的迭代器是 const char* → std::cregex_iterator（cmatch）
 */
template <typename Text>
Range(const Text &, const std::regex &)
    -> Range<std::regex_iterator<std::ranges::iterator_t<const Text &>>>;

/** @brief 推导指引：字符迭代器对 + 正则（如 s.data() / sv.begin() 的显式区间） */
template <typename It>
Range(It, It, const std::regex &) -> Range<std::regex_iterator<It>>;

/** @brief 推导指引：C 字符串与字符数组不是 range，iterator_t 推不出来，需单独给出
 *  @note 推导指引只参与推导，不参与重载决议，故形参按引用/按值写都不会额外拷贝；
 *        字符数组版用 const 引用，否则字符串字面量（const char[N]）绑不上
 */
Range(const char *, const std::regex &) -> Range<std::cregex_iterator>;
Range(const char *, const char *, const std::regex &) -> Range<std::cregex_iterator>;

template <std::size_t N>
Range(const char (&)[N], const std::regex &) -> Range<std::cregex_iterator>;

NAMESPACE_END
