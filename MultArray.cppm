/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/10/16 22:59
*******************************************************************************/

module;
#include <span>
#include <stdexcept>
#include <cassert>
#include <utility>
#include <vector>

#include "tools.h"
export module MultArray;

export
NAMESPACE_BEGIN(nl)

template<typename T,size_t N = 2>
class

#if __cplusplus >= 202302L
[[deprecated("plase use mdspan")]]
#endif
MultArray {
    T* data_;
    size_t count_;
    std::vector<size_t> dimensions_;
public:
    MultArray(T* p,const std::vector<size_t> &dims) : data_(p), dimensions_(std::move(dims)) {
        count_ = 1;
        for (const int val : dimensions_)
            count_ *= val;
    }

    T& operator()(const std::vector<int> &indices) {
        auto index = 0;
        for (size_t i = 0; i < indices.size(); ++i)
            index = index * dimensions_[i] + indices[i];
        return data_[index];
    }

    [[nodiscard]]
    size_t size() const {
        return dimensions_[0];
    }

    MultArray operator[] (const size_t index) {
        return MultArray(data_ + index * count_ / dimensions_[0],std::vector(dimensions_.begin() + 1,dimensions_.end()));
    }

    std::span<T> to_span() {
        return std::span<T>(data_, count_);
    }

};


template<typename T>
class MultArray<T,2> {
    T* data_{};
    int row_{}, col_{};
    size_t count_{};
public:
    MultArray() {  }

    MultArray(T* p, int row, int col) : data_(p) , row_(row), col_(col) , count_(row * col) {  }

    MultArray(const MultArray &arr) {
        data_ = arr.data_;
        row_ = arr.row_;
        col_ = arr.col_;
        count_ = arr.count_;
    }
    MultArray& operator= (const MultArray &arr) {
        new (this) MultArray(arr);
        return *this;
    }

    T& operator()(int row,int col) {
        return *(data_ + row * col_ + col);
    }
    T& at(int row, int col) {
        assert(row * col < count_ && "index out of range");
        return data_[row * col_ + col];
    }

    T *operator[] (const size_t row) {
        return data_ + row * col_;
    }
    const T* operator[] (const size_t row) const {
        return data_ + row * col_;
    }

};



NAMESPACE_END(nl)