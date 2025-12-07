/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/01 22:56
*******************************************************************************/
export module modforge.average_queue;

import std;

template <typename T>
concept CanAverageType = requires(T val1, T val2, T res, int count){
    T {};
    res = val1 + val2;
    res /= count;
};

export
template <CanAverageType T, std::size_t N>
class AverageQueue {
    int MAX_SIZE = N + 1;
    std::array<T, N + 1> data_;
    int front_{}, rear_{};
public:
    AverageQueue() = default;
    AverageQueue(const std::initializer_list<T> &init_list) {
        if (init_list.size() > N)
            throw std::runtime_error(std::format("Can't have more than {}", N));

        for (const auto& item : init_list) {
            this->push(item);
        }
    }

    void push(const T &val) {
        const int rear_back = rear_;
        data_[rear_++] = val;
        rear_ %= MAX_SIZE;

        if ((rear_back + 1) % MAX_SIZE == front_) {
            front_++;
            front_ %= MAX_SIZE;
        }
    }

    T average() {
        int size = this->size();
        if (size == 0)
            return {};

        T ret{};
        for (int i = 0; i < size; ++i) {
            int pos = (front_ + i) % MAX_SIZE;
            ret += data_[pos];
        }
        return ret / size;
    }


    [[nodiscard]]
    std::size_t size() const {
        if (rear_ >= front_)
            return rear_ - front_;
        return MAX_SIZE - front_ + rear_;
    }
};
