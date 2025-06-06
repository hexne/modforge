/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <vector>
#include <memory>
#include <mdspan>
export module modforge.tensor;

export
template<typename T, size_t extents>
class Tensor {
    std::shared_ptr<T[]> data_;
    std::mdspan<T, std::dextents<size_t, extents>, std::layout_stride> view_;

    template<typename ... Args>
    constexpr int mul(Args && ...args) {
        return (... * args);
    }

    template<size_t size>
    auto to_vector_impl() {
        if constexpr (size == 1) {
            return std::vector<T>(view_.extent(extents - 1));
        }
        else {
            return std::vector(view_.extent(extents - size), to_vector_impl<size - 1>());
        }
    }


    template<typename U, typename ... Args>
    [[nodiscard]]
    auto& access_vector(std::vector<U>& vec, int first, Args &&...args) {
        if constexpr (sizeof ...(args) >= 1)
            return access_vector(vec[first], args ...);
        else
            return vec[first];
    }

    template<typename Vector,typename ... Index>
    void copy_to_vector_impl(Vector &vec, Index &&...index) {
        if constexpr (sizeof...(index) == extents) {
            access_vector(vec, index...) = view_[index ...];
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                copy_to_vector_impl(vec, index..., i);
            }
        }
    }
    template<typename Mdspan,typename OP, typename ... Index>
    void traversal_mdspan_impl(Mdspan &other, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == extents) {
            view_[index ...] = op(view_[index ...], other.view_[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                traversal_mdspan_impl(other, op, index..., i);
            }
        }
    }

public:
    int x{}, y{}, z{};

    Tensor() = default;
    ~Tensor() = default;

    Tensor(T *ptr, size_t count, auto &&... args) requires (sizeof ...(args) == extents) {

        data_ = std::make_shared<T[]>(count);
        std::copy(ptr, ptr + count, data_.get());
        view_ = std::mdspan(data_.get(), args ...);

        if constexpr (sizeof...(args) == 1) {
            y = view_.extent(0);
        }
        else if constexpr (sizeof...(args) == 2) {
            x = view_.extent(0);
            y = view_.extent(1);
        }
        else if constexpr (sizeof...(args) == 3) {
            z = view_.extent(0);
            x = view_.extent(1);
            y = view_.extent(2);
        }
    }

    Tensor(T *ptr, size_t count, std::layout_stride::mapping<std::dextents<size_t, extents>> mapping) {

        data_ = std::make_shared<T[]>(count);
        std::copy(ptr, ptr + count, data_.get());
        view_ = std::mdspan(data_.get(), mapping);

        if constexpr (extents == 1) {
            y = view_.extent(0);
        }
        else if constexpr (extents == 2) {
            x = view_.extent(0);
            y = view_.extent(1);
        }
        else if constexpr (extents == 3) {
            z = view_.extent(0);
            x = view_.extent(1);
            y = view_.extent(2);
        }
    }
    Tensor(std::shared_ptr<T[]> ptr, std::layout_stride::mapping<std::dextents<size_t, extents>> mapping) : data_(ptr), view_(ptr, mapping) {
        if constexpr (extents == 1) {
            y = view_.extent(0);
        }
        else if constexpr (extents == 2) {
            x = view_.extent(0);
            y = view_.extent(1);
        }
        else if constexpr (extents == 3) {
            z = view_.extent(0);
            x = view_.extent(1);
            y = view_.extent(2);
        }
    }

    Tensor(std::shared_ptr<T[]> ptr, auto &&... args) requires (sizeof ...(args) == extents)
                            : Tensor(ptr.get(), args ...) {
        data_ = ptr;
    }


    /*******************************************************************************
     * 拥有数据的构造函数
    *******************************************************************************/
    Tensor(auto &&... args) requires (sizeof...(args) == extents)
                            : Tensor(std::make_shared<T[]>(mul(args ...)), args ...) {  }

    Tensor(const std::vector<T> & nums, auto && ...args) requires (sizeof...(args) == extents)
                            : Tensor(std::make_shared<T[]>(nums.size()), args ...) {
        std::copy(nums.begin(), nums.end(), data_.get());
    }

    Tensor(const std::initializer_list<T> & nums, auto && ...args) requires (sizeof...(args) == extents)
                            : Tensor(std::vector<T>{nums}, args ...) {  }

    // 对于拷贝构造函数, 不构造数据
    Tensor(const Tensor &tensor) = default;
    Tensor& operator = (const Tensor & Tensor) = default;

    Tensor(Tensor && Tensor) = default;
    Tensor& operator = (Tensor && Tensor) = default;


    Tensor copy() {

        auto data = std::make_shared<T[]>(data_.use_count());
        std::copy(data_.get(), data_.get() + data_.use_count(), data.get());

        return Tensor(data, view_.mapping());
    }


    auto extent(int extent) {
        return view_.extent(extent);
    }

    T& operator[](auto && ...args) requires (std::is_same_v<std::remove_cvref_t<decltype(args)>, int> && ...) {
        return view_[std::forward<decltype(args)>(args)...];
    }

    auto to_vector() {
        auto vec = to_vector_impl<extents>();
        copy_to_vector_impl(vec);
        return vec;
    }

    Tensor operator + (const Tensor &tensor) {

        if (view_.rank() != tensor.view_.rank() || view_.extents() != tensor.view_.extents()) {
            throw std::runtime_error("Tensor::operator +");
        }

        Tensor ret(*this);

        traversal_mdspan_impl(tensor, [](T& val1, T& val2) {
            return val1 + val2;
        });

        return ret;
    }
    Tensor operator - (const Tensor &tensor) {
        return {};
    }
    Tensor operator * (const Tensor &tensor) {
        return {};
    }
    Tensor& operator += (const Tensor &tensor) {





        return *this;
    }
    Tensor& operator -= (const Tensor &tensor) {
        return *this;
    }
    Tensor& operator *= (const Tensor &tensor) {
        return *this;
    }


};


template<typename T, typename ... Extents>
Tensor(T *, size_t, Extents ... extents) -> Tensor<T, sizeof ...(Extents)>;

template<typename T, typename ... Extents>
Tensor(std::shared_ptr<T[]> ptr, Extents ... extents) -> Tensor<T, extents...>;