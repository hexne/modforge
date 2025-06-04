/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <iostream>
#include <vector>
#include <random>
#include <memory>
#include <mdspan>
export module modforge.matrix;

export
template<typename T, size_t extents>
class Matrix {
    std::shared_ptr<T[]> data_;
    std::mdspan<T, std::dextents<size_t, extents>, std::layout_stride> view_;

    template<typename ... Args>
    constexpr int Mul(Args && ...args) {
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
    void copy_impl(Vector &vec, Index &&...index) {
        if constexpr (sizeof...(index) == extents) {
            access_vector(vec, index...) = view_[index ...];
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                copy_impl(vec, index..., i);
            }
        }
    }


public:
    int x{}, y{}, z{};

    Matrix() = default;
    ~Matrix() = default;


    /*******************************************************************************
     * 不拥有数据的构造函数
    *******************************************************************************/
    Matrix(T *ptr, auto &&... args) requires (sizeof ...(args) == extents) : view_(std::mdspan(ptr, args ...)) {
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

    Matrix(T *ptr, std::layout_stride::mapping<std::dextents<size_t, extents>> mapping) : view_(ptr, mapping) {
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

    Matrix(std::shared_ptr<T[]> ptr, auto &&... args) requires (sizeof ...(args) == extents)
                            : Matrix(ptr.get(), args ...) {
        data_ = ptr;
    }

    Matrix(std::mdspan<T, std::dextents<size_t, extents>, std::layout_stride> view) : view_(view) {
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

    /*******************************************************************************
     * 拥有数据的构造函数
    *******************************************************************************/
    Matrix(auto &&... args) requires (sizeof...(args) == extents)
                            : Matrix(std::make_shared<T[]>(Mul(args ...)), args ...) {  }

    Matrix(const std::vector<T> & nums, auto && ...args) requires (sizeof...(args) == extents)
                            : Matrix(std::make_shared<T[]>(nums.size()), args ...) {
        std::copy(nums.begin(), nums.end(), data_.get());
    }

    Matrix(const std::initializer_list<T> & nums, auto && ...args) requires (sizeof...(args) == extents)
                            : Matrix(std::vector<T>{nums}, args ...) {  }

    Matrix(const Matrix & matrix) = default;
    Matrix(Matrix && matrix) = default;
    Matrix& operator = (const Matrix & matrix) = default;
    Matrix& operator = (Matrix && matrix) = default;

    auto extent(int extent) {
        return view_.extent(extent);
    }

    T& operator[](auto && ...args) requires (std::is_same_v<std::remove_cvref_t<decltype(args)>, int> && ...) {
        return view_[std::forward<decltype(args)>(args)...];
    }



    auto to_vector() {
        auto vec = to_vector_impl<extents>();
        copy_impl(vec);
        return vec;
    }

};
//
// export
// template <typename T>
// class Matrix<T, 5> {
//     std::shared_ptr<T[]> data_ptr_;
//     std::mdspan<T, std::dextents<size_t, 3>> view_;
//
//     friend std::ostream& operator<<(std::ostream& os, const Matrix<T>& matrix) {
//         os << "[\n";
//
//         for (int z = 0;z < matrix.z; ++z) {
//             std::cout << "\t[\n";
//
//             for (int x = 0; x < matrix.x; ++x) {
//                 std::cout << "\t";
//                 for (int y = 0; y < matrix.y; ++y) {
//                     std::cout << matrix.get_const(x, y, z) << " ";
//                 }
//                 std::cout << '\n';
//             }
//             std::cout << "\t]\n";
//         }
//         os << "\n]";
//         std::endl(std::cout);
//         return os;
//     }
// public:
//     operator std::vector<T> () {
//         std::vector<T> ret(x * y * z);
//         int count{};
//         for (int i = 0;i < z; ++i)
//             for (int j = 0;j < x; ++j)
//                 for (int k = 0;k < y; ++k)
//                     ret[count++] = this->operator[](j, k, i);
//         return ret;
//     }
//
//
//     // 随机初始化到 [-1.f, 1.f]
//     void random_init() {
//         for (int i = 0; i < x * y * z; ++i)
//             data_ptr_[i] = dis(gen);
//     }
//
//     // msvc暂不支持 operator [] 存在默认参数
//     T& operator[] (size_t x, size_t y) {
//         size_t z = 0uz;
//         if (z * this->x * this->y + x * this->y + y >= this->z * this->x * this->y) {
//             std::cout << "out of range" << std::endl;
//         }
//         return view_[z, x, y];
//     }
//     T& operator[] (size_t x, size_t y, size_t z) {
//         if (z * this->x * this->y + x * this->y + y >= this->z * this->x * this->y) {
//             std::cout << "out of range" << std::endl;
//         }
//         return view_[z, x, y];
//     }
//     const T &get_const(size_t x, size_t y, size_t z = 0) const {
//         return view_[z, x, y];
//     }
//
//     size_t extent(size_t index) {
//         return view_.extent(index);
//     }
//
//     Matrix operator + (const Matrix & right) const {
//
//         if (x != right.x || y != right.y || z != right.z) {
//             throw std::runtime_error("Matrix::operator +");
//         }
//
//         Matrix ret(x, y, z);
//
//         for (int i = 0; i < x; ++i) {
//             for (int j = 0; j < y; ++j) {
//                 for (int k = 0; k < z; ++k) {
//                     ret[i, j, k] = get_const(i, j, k) + right.get_const(i, j, k);
//                 }
//
//             }
//         }
//         return ret;
//     }
//
//     Matrix operator - (const Matrix & right) const {
//
//         if (x != right.x || y != right.y || z != right.z) {
//             throw std::runtime_error("Matrix::operator -");
//         }
//
//         Matrix ret(x, y, z);
//
//         for (int i = 0; i < x; ++i) {
//             for (int j = 0; j < y; ++j) {
//                 for (int k = 0; k < z; ++k) {
//                     ret[i, j, k] = get_const(i, j, k) - right.get_const(i, j, k);
//                 }
//
//             }
//         }
//         return ret;
//     }
//
//     Matrix operator * (const Matrix & right) const {
//         if (y != right.x || z != right.z)
//             throw std::runtime_error("can't multiply matrix");
//
//         Matrix ret(x, right.y, z);
//         for (int k = 0;k < z; ++k) {
//
//             for (int i = 0; i < x; i++) {
//                 for (int j = 0; j < right.y; j++) {
//                     ret[i, j, k] = 0;
//                     for (int t = 0; t < y; t++)
//                         ret[i ,j, k] += get_const(i, t, k) * right.get_const(t, j, k);
//                 }
//             }
//
//         }
//         return ret;
//     }
//
//     Matrix& operator += (const Matrix & right) {
//         if (x != right.x || y != right.y || z != right.z) {
//             throw std::runtime_error("Matrix::operator +");
//         }
//         for (int i = 0; i < x; ++i) {
//             for (int j = 0; j < y; ++j) {
//                 for (int k = 0; k < z; ++k) {
//                     operator[](i, j, k) = get_const(i, j, k) + right.get_const(i, j, k);
//                 }
//             }
//         }
//         return *this;
//     }
//     Matrix& operator -= (const Matrix & right) {
//         if (x != right.x || y != right.y || z != right.z) {
//             throw std::runtime_error("Matrix::operator -");
//         }
//         for (int i = 0; i < x; ++i) {
//             for (int j = 0; j < y; ++j) {
//                 for (int k = 0; k < z; ++k) {
//                     operator[](i, j, k) = get_const(i, j, k) - right.get_const(i, j, k);
//                 }
//             }
//         }
//         return *this;
//     }
//
//     Matrix& operator *= (const Matrix & right) {
//         if (y != right.x || z != right.z)
//             throw std::runtime_error("can't multiply matrix");
//
//         Matrix ret(x, right.y, z);
//         for (int k = 0;k < z; ++k) {
//
//             for (int i = 0; i < x; i++) {
//                 for (int j = 0; j < right.y; j++) {
//                     ret[i, j, k] = 0;
//                     for (int t = 0; t < y; t++)
//                         ret[i ,j, k] += get_const(i, t, k) * right.get_const(t, j, k);
//                 }
//             }
//
//         }
//         *this = std::move(ret);
//         return *this;
//     }
// };