/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <functional>
#include <vector>
#include <format>
#include <memory>
#include <mdspan>
#include <stdexcept>
#include <iostream>
export module modforge.tensor;

export
template <typename T, size_t Extents>
class Tensor;

export
template<typename T>
class Vector {
    std::shared_ptr<std::vector<T>> data_;

public:
    Vector() = default;
    explicit Vector(int n) : data_(std::make_shared<std::vector<T>>(n)) {  }
    Vector(T *ptr, size_t size) : data_(std::make_shared<std::vector<T>>(size)) {  }

    Vector(const Vector &) = default;
    Vector(Vector &&) = default;

    Vector &operator = (const Vector &) = default;
    Vector &operator = (Vector &&) = default;

    Vector operator - (const Vector &right) const {
        if (data_->size() != right.data_->size())
            throw std::runtime_error("Vector size mismatch");

        Vector ret(data_->size());
        for (int i = 0;i < data_->size();i++)
            ret[i] =  data_->operator[](i) - right[i];

        return ret;
    }

    Vector operator *(const Tensor<T, 2> &tensor) {
        if (data_->size() != tensor.extent(0)) {
            throw std::runtime_error(std::format("{}x{} cant * {}x{}", 1, data_->size(), tensor.extent(0), tensor.extent(1)));
        }

        Vector ret(tensor.extent(1));

        for (int y = 0; y < tensor.extent(1); ++y) {
            T res{};
            for (int x = 0; x < tensor.extent(0); ++x) {
                res += (*data_)[x] * tensor[x, y];
            }
            ret[y] = res;
        }
        return ret;
    }
    Vector &operator *=(const Tensor<T, 2> &tensor) {
        auto ret = operator*(tensor);
        *this = ret;
        return *this;
    }

	T& operator[](size_t index) {
		return (*data_)[index];
	}
    T operator[](size_t index) const {
        return (*data_)[index];
    }

    size_t size() const {
        if (data_)
            return data_->size();
        return 0;
    }

    void foreach(std::function<void(T &)> func) {
        for (auto &val : data_)
            func(val);
    }

    void read(std::istream &in) {
        int size{};
        in.read(reinterpret_cast<char *>(&size), sizeof(size));
        data_ = std::make_shared<std::vector<T>>(size);
    }
    void write(std::ostream &out) const {
        int size = data_->size();
        out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    }

    friend std::istream &operator >> (std::istream &in, Vector &vec) {
        vec.read(in);
        return in;
    }
    friend std::ostream &operator << (std::ostream &out, Vector &vec) {
        vec.write(out);
        return out;
    }

};

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
    // - +
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
    // -= +=
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

    // foreach
    template <typename Mdspan, typename OP, typename ... Index>
    void foreach_impl(Mdspan &mdspan, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            op(view_[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                foreach_impl(mdspan, op, index..., i);
            }
        }

    }

    template <size_t N, typename ... Index>
    void mul_impl(Tensor &result, const Tensor &other, Index ...index) const {
        // 此处计算乘法
        if constexpr (N == Extents - 2) {
            const size_t n = view_.extent(Extents - 2); // 结果行数
            const size_t p = other.view_.extent(Extents - 1); // 结果列数
            const size_t k = view_.extent(Extents - 1); // 内积维度

            for (size_t i = 0; i < n; ++i) {
                for (size_t j = 0; j < p; ++j) {
                    T sum{};
                    for (size_t l = 0; l < k; ++l) {
                        sum += view_[index..., i, l] * other.view_[index..., l, j];
                    }
                    result.view_[index..., i, j] = sum;
                }
            }
        }
        else {
            for (int i = 0;i < view_.extent(N); ++i) {
                mul_impl<N + 1>(result, other, index..., i);
            }

        }


    }

    template<size_t... I>
    void create_tensor(std::vector<int>& dims, std::index_sequence<I...>) {
        new (this) Tensor(dims[I]...);
    }
public:
    Tensor() = default;

    Tensor(auto && ...args) requires (sizeof ...(args) == Extents) {
        data_ = std::make_shared<std::vector<T>>(mul(args ...));
        view_ = std::mdspan(data_->data(), args ...);
    }

    // 不拥有数据的所有权，为了让view使用更方便，该接口留给view使用
    static Tensor from_view(const T* data, auto && ...args) requires (sizeof ...(args) == Extents) {
        Tensor<T, sizeof ...(args)> ret;
        ret.view_ = std::mdspan(data, args ...);
        return ret;
    }
    static Tensor from_view(const T* data, std::layout_stride::mapping<std::dextents<size_t, Extents>> mapping) {
        Tensor ret;
        ret.view_ = std::mdspan(data, mapping);
        return ret;
    }
    static Tensor<T, Extents> from_view(Tensor<T, Extents> &tensor) {
        Tensor ret;
        ret.view_ = tensor.view_;
        int x = ret.view_.extent(0);
        int y = ret.view_.extent(1);
        return ret;
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

    void read(std::istream &in) {
        int rank;
        in.read(reinterpret_cast<char*>(&rank),sizeof(int));
        std::vector<int> dims(rank);
        for (int i = 0;i < rank; ++i) {
            in.read(reinterpret_cast<char*>(&dims[i]),sizeof(int));
        }

        create_tensor(dims, std::make_index_sequence<Extents>{});
        this->foreach([&](auto &val) {
            in.read(reinterpret_cast<char*>(&val),sizeof(val));
        });

    }
    void write(std::ostream &out) {
        // 维度
        int rank = view_.rank();
        out.write(reinterpret_cast<const char*>(&rank), sizeof(rank));
        // 依次保存维度
        for (int i = 0; i < rank; ++i) {
            int cur_size = view_.extent(i);
            out.write(reinterpret_cast<const char*>(&cur_size), sizeof(cur_size));
        }

        // 保存剩下的数据
        this->foreach([&] (auto &val){
            out.write(reinterpret_cast<char*>(&val), sizeof(val));
        });

    }

    friend std::istream &operator >> (std::istream &in, Tensor &tensor) {
        tensor.read(in);
        return in;
    }
    friend std::ostream &operator << (std::ostream &out, Tensor &tensor) {
        tensor.write(out);
        return out;
    }


    Tensor copy() const {
        auto ptr = std::make_shared<std::vector<T>>(data_->size());
        std::copy(data_->begin(), data_->end(), ptr->begin());
        return Tensor(ptr, view_.mapping());
    }

    Tensor copy_size() const {
        auto ptr = std::make_shared<std::vector<T>>(data_->size());
        return Tensor(ptr, view_.mapping());
    }

    [[nodiscard]] size_t extent(size_t extent) const {
        return view_.extent(extent);
    }

    [[nodiscard]] size_t rank() const {
        return view_.rank();
    }

    void foreach(std::function<void (T &)> func) {
        foreach_impl(view_, func);
    }

    auto to_vector(bool only_size = false) const {
        auto vec = to_vector_impl<Extents>();
        if (!only_size)
            copy_to_vector_impl(vec);
        return vec;
    }

    T& operator [] (auto && ...index) requires (sizeof ...(index) == Extents) {
        return view_[index ...];
    }

    T operator [] (auto && ...index) const requires (sizeof ...(index) == Extents) {
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
        if (view_.rank() != other.view_.rank())
            throw std::runtime_error("Tensor rank is different");

        for (int i = 0;i < view_.rank() - 2; ++i) {
            if (view_.extent(i) != other.view_.extent(i))
                throw std::runtime_error("Tensor size is different");
        }

        if (view_.extent(Extents - 1) != other.view_.extent(Extents - 2))
            throw std::runtime_error("Tensor size is different");


        std::array<size_t, Extents> new_extents;
        for (int i = 0;i < Extents - 2; ++i)
            new_extents[i] = view_.extent(i);
        new_extents[Extents - 2] = view_.extent(Extents - 2);
        new_extents[Extents - 1] = other.view_.extent(Extents - 1);
        auto create = [&]<size_t ...Index>(std::index_sequence<Index...>) {
            return Tensor<T, Extents>(new_extents[Index]...);
        };
        auto result = create(std::make_index_sequence<Extents>());

        mul_impl<0>(result, other);
        return result;
    }
    Tensor& operator *= (const Tensor &other) {
        auto res = operator*(other);
        *this = std::move(res);
        return *this;
    }

    // 转置
    Tensor& transpose() {
        static_assert(Extents >= 2, "Transpose requires at least 2 dimensions");

        // 新的步长
        std::array<size_t, Extents> strides;
        for (size_t i = 0; i < Extents - 2; ++i)
            strides[i] = view_.mapping().stride(i);
        strides[Extents - 2] = view_.mapping().stride(Extents - 1);
        strides[Extents - 1] = view_.mapping().stride(Extents - 2);

        // 新的维度
        std::array<size_t, Extents> extents;
        for (size_t i = 0; i < Extents; ++i)
            extents[i] = view_.extent(i);
        std::swap(extents[Extents - 2], extents[Extents - 1]);

        // 构造 extents 对象
        auto new_extents = [&]{
            if constexpr (Extents == 1) {
                return std::dextents<size_t, 1>{extents[0]};
            } else if constexpr (Extents == 2) {
                return std::dextents<size_t, 2>{extents[0], extents[1]};
            } else if constexpr (Extents == 3) {
                return std::dextents<size_t, 3>{extents[0], extents[1], extents[2]};
            } else {
                static_assert(Extents <= 3, "Only support up to 3 dimensions");
                return std::dextents<size_t, Extents>{};
            }
        }();

        view_ = std::mdspan(view_.data_handle(),
                           std::layout_stride::mapping(new_extents, strides));

        return *this;
    }


};

template <typename T, typename ...Args>
Tensor(T *, size_t, Args ...) -> Tensor<T, sizeof...(Args)>;

template <typename T, typename ...Args>
Tensor(const std::vector<T> &vec, Args ...) -> Tensor<T, sizeof...(Args)>;

template <typename T, typename ...Args>
Tensor(const std::initializer_list<T> &vec, Args ...) -> Tensor<T, sizeof...(Args)>;