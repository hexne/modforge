/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <vector>
#include <memory>
#include <mdspan>
#include <stdexcept>
export module modforge.tensor;

export
template <typename T,size_t Extents>
class Tensor {
    std::shared_ptr<std::vector<T>> data_;
    std::mdspan<T, std::dextents<size_t, Extents>, std::layout_stride> view_;

    template<typename ... Args>
    constexpr int mul(Args && ...args) {
        return (... * args);
    }


    template<typename U, typename ... Args>
    [[nodiscard]]
    auto& access_vector(std::vector<U>& vec, size_t first, Args &&...index) {
        if constexpr (sizeof ...(index) >= 1)
            return access_vector(vec[first], index ...);
        else
            return vec[first];
    }

    template<size_t extent>
    auto to_vector_impl() {
        if constexpr (extent == 1) {
            return std::vector<T>(view_.extent(Extents - 1));
        }
        else {
            return std::vector(view_.extent(Extents - extent), to_vector_impl<extent - 1>());
        }
    }

    template<typename Vector,typename ... Index>
    void copy_to_vector_impl(Vector &vec, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            access_vector(vec, index...) = view_[index ...];
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                copy_to_vector_impl(vec, index..., i);
            }
        }
    }

    template<typename Mdspan,typename OP, typename ... Index>
    void traversal_mdspan_impl(Mdspan &mdspan1, Mdspan &mdspan2, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            view_[index ...] = op(mdspan1[index ...], mdspan2[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                traversal_mdspan_impl(mdspan1, mdspan2, op, index..., i);
            }
        }
    }
    template<typename Mdspan,typename OP, typename ... Index>
    void traversal_mdspan_impl(Mdspan &mdspan, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            op(view_[index ...], mdspan[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                traversal_mdspan_impl(mdspan, op, index..., i);
            }
        }
    }

public:
    Tensor(auto && ...args) requires (sizeof ...(args) == Extents) {
        data_ = std::make_shared<std::vector<T>>(mul(args ...));
        view_ = std::mdspan(data_->data(), args ...);
    }
    Tensor(const T *data, size_t count, auto && ...args) requires (sizeof ...(args) == Extents) {
        data_ = std::make_shared<std::vector<T>>(count);
        std::copy(data, data + count, data_->begin());
        view_ = std::mdspan(data_->data(), args ...);
    }

    Tensor(std::shared_ptr<T[]> ptr,size_t count, auto && ...args) requires (sizeof ...(args) == Extents)
                : Tensor(ptr.get(), count, std::forward<decltype(args)>(args) ...) {

    }
    explicit Tensor(const std::vector<T> &vec, auto && ...args) requires (sizeof ...(args) == Extents) : Tensor(std::data(vec), vec.size(), args ...) {  }
    Tensor(std::shared_ptr<std::vector<T>> vec, std::layout_stride::mapping<std::dextents<size_t, Extents>> mapping) : data_(vec), view_(data_->data(), mapping) {  }
    Tensor(const std::initializer_list<T> &vec, auto && ...args) requires (sizeof ...(args) == Extents) : Tensor(std::data(vec), vec.size(), args ...) {  }

    Tensor(const Tensor &) = default;
    Tensor& operator = (const Tensor &) = default;

    Tensor(Tensor &&) = default;
    Tensor& operator = (Tensor &&) = default;


    Tensor copy() const {
        auto ptr = std::make_shared<std::vector<T>>(data_->size());
        std::copy(data_->begin(), data_->end(), ptr->begin());
        return Tensor(ptr, view_.mapping());
    }

    Tensor copy_size() const {
        auto ptr = std::make_shared<std::vector<T>>(data_->size());
        return Tensor(ptr, view_.mapping());
    }

    size_t extent(size_t extent) {
        return view_.extent(extent);
    }

    auto to_vector(bool only_size = false) {
        auto vec = to_vector_impl<Extents>();
        if (!only_size)
            copy_to_vector_impl(vec);
        return vec;
    }

    T& operator [] (auto && ...index) requires (sizeof ...(index) == Extents) {
        return view_[index ...];
    }


    constexpr bool check_size(const Tensor &other) const {
        for (int i = 0;i < view_.rank(); ++i)
            if (view_.extent(i) != other.view_.extent(i))
                return false;
        return true;
    }
    Tensor operator + (const Tensor &other) const {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        auto ret = other.copy_size();
        ret.traversal_mdspan_impl(view_, other.view_, [] (auto val1, auto val2){
            return val1 + val2;
        });
        return ret;
    }
    Tensor operator - (const Tensor &other) const {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        auto ret = other.copy_size();
        ret.traversal_mdspan_impl(view_, other.view_, [] (auto val1, auto val2){
            return val1 - val2;
        });
        return ret;
    }

    Tensor &operator += (const Tensor &other) {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        traversal_mdspan_impl(other.view_, [] (auto &val1, auto &val2){
            return val1 += val2;
        });
        return *this;
    }
    Tensor &operator -= (const Tensor &other) {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        traversal_mdspan_impl(other.view_, [] (auto &val1, auto &val2){
            return val1 -= val2;
        });
        return *this;
    }
    Tensor operator * (const Tensor &other) const {

    }
    Tensor operator *= (const Tensor &other) const {

    }



};


template <typename T, typename ...Args>
Tensor(T *, size_t, Args ...) -> Tensor<T, sizeof...(Args)>;

template <typename T, typename ...Args>
Tensor(const std::vector<T> &vec, Args ...) -> Tensor<T, sizeof...(Args)>;
template <typename T, typename ...Args>
Tensor(const std::initializer_list<T> &vec, Args ...) -> Tensor<T, sizeof...(Args)>;



export
template <typename T, size_t Extents>
class TensorView {
    std::mdspan<T, std::dextents<size_t, Extents>, std::layout_stride> view_;
public:
    explicit TensorView(Tensor<T, Extents> & tensor) : view_(tensor.view_) {  }

    size_t extent(size_t extent) {
        return view_.extent(extent);
    }

    T& operator[](auto && ...index) requires (sizeof ...(index) == Extents) {
        return view_[index ...];
    }

};