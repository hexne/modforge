/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/01 22:56
*******************************************************************************/
export module modforge.average_queue;

import std;

template <typename T>
concept AverageType = requires(T val1, T val2, T res, int count){
	T {};
	res = val1 + val2;
	res = val1 - val2;
	res /= count;
};

export
template <AverageType T, std::size_t MaxSize>
class AverageQueue {
	std::array<T, MaxSize + 1> data_{};
	std::size_t max_size_ = MaxSize + 1;
	int front_{}, rear_{};
public:

    AverageQueue() = default;

	AverageQueue(const std::initializer_list<T> &init_list) {
		if (init_list.size() > max_size_)
			throw std::invalid_argument("AverageQueue is too many elements");

		front_ = 0;
		rear_ = init_list.size();
		copy(init_list.begin(), init_list.end(), data_.begin());
	}

	void push_back(const T &val) {
		int rear_back = rear_;
		data_[rear_++] = val;
		rear_ %= max_size_;

		if ((rear_back + 1) % max_size_ == front_) {
			front_++;
			front_ %= max_size_;
		}
	}

    T get_average() {
	    int size = this->size();
	    T ret{};
	    for (int i = 0; i < size; ++i) {
	        int pos = (front_ + i) % max_size_;
	        ret += data_[pos];
	    }
	    return ret / size;
	}


	std::size_t size() const {
		int size = rear_ - front_;
		if (size > 0)
			return size;
		return size + max_size_;
	}

};