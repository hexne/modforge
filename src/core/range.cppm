/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 15:33:24
********************************************************************************/

module;
export module modforge.range;
import std;



template <typename T>
struct IsContainer : std::false_type { };

template <typename T>
    requires requires (T t) { t.begin(); t.end(); t.size(); }
struct IsContainer<T> : std::true_type { };


export template <typename T>
class Range;


template <typename T>
    requires std::is_integral_v<T>
class Range<T>{
public:
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
    explicit Range(int count) : Range(0, count) {  }
    Range(int begin, int end) : begin_(iterator(begin)), end_(iterator(end)) {  }
    iterator begin() {
        return begin_;
    }
    iterator end() {
        return end_;
    }
    [[nodiscard]] std::size_t distance() const {
        return end_ - begin_;
    }
private:
    iterator begin_, end_;
};

template <typename T>
    requires std::is_pointer_v<T>
class Range<T> {
public:
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
    Range(T begin_pointer, T end_pointer) : begin_(begin_pointer), end_(end_pointer) {  }
    iterator begin() {
        return begin_;
    }
    iterator end() {
        return end_;
    }
    std::size_t distance() {
        return end_ - begin_;
    }
private:
    iterator begin_, end_;
};

template <typename T>
    requires std::is_array_v<T>
class Range<T> {
    using PT = std::decay_t<T>;
    using Iterator = typename Range<PT>::iterator;
    Iterator begin_{}, end_{};
public:
    explicit Range(T &arr) : begin_(std::begin(arr)), end_(std::end(arr)) { }
    Iterator begin() {
        return begin_;
    }
    Iterator end() {
        return end_;
    }
    std::size_t distance() {
        return end_ - begin_;
    }

};



template <typename T>
    requires IsContainer<T>::value
class Range<T> {
    T container_;
public:
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

    explicit Range(T &container) :
        container_(container), begin_(container.begin()), end_(container.end()) {  }
    auto begin() {
        return begin_;
    }
    auto end() {
        return end_;
    }
    std::size_t distance() {
        return container_.size();
    }
private:
    iterator begin_, end_;
};


Range(int) -> Range<int>;

// 数字和指针
template <typename T>
Range(T, T) -> Range<T>;

// 容器和数组
template <typename T>
Range(T &) -> Range<T>;

