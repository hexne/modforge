/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>
#include <memory>
#include <mdspan>

#include "tools.h"
export module Matrix;

static std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<> dis(-1.f, 1.f);

export
NAMESPACE_BEGIN(nl)

template <typename T = float>
class Matrix {
    std::shared_ptr<T[]> data_ptr_;
    std::mdspan<T, std::dextents<size_t, 3>> view_;

    friend std::ostream& operator<<(std::ostream& os, const Matrix<T>& matrix) {
        os << "[\n";

        for (int z = 0;z < matrix.z; ++z) {
            std::cout << "\t[\n";

            for (int x = 0; x < matrix.x; ++x) {
                std::cout << "\t";
                for (int y = 0; y < matrix.y; ++y) {
                    std::cout << matrix.get_const(x, y, z) << " ";
                }
                std::cout << '\n';
            }
            std::cout << "\t]\n";
        }
        os << "\n]";
        std::endl(std::cout);
        return os;
    }

public:
    int x{}, y{}, z{};
    Matrix() = default;

    // 有所有权
    Matrix(int x, int y, int z = 1) : Matrix(std::make_shared<T[]>(x * y * z), x, y, z) {  }

    Matrix(std::shared_ptr<T[]> ptr, int x, int y, int z = 1) : x(x), y(y), z(z),
                            data_ptr_(ptr) , view_(std::mdspan(data_ptr_.get(), z, x, y)) {  }

    Matrix(const Matrix &right) {
        *this = right;
    }
    Matrix(Matrix &&right)  noexcept {
        *this = std::move(right);
    }
    Matrix& operator = (const Matrix& matrix) {
        data_ptr_ = matrix.data_ptr_;
        view_ = matrix.view_;
        x = matrix.x;
        y = matrix.y;
        z = matrix.z;
        return *this;
    }
    Matrix& operator = (Matrix&& right) {
        data_ptr_ = std::move(right.data_ptr_);
        view_ = std::move(right.view_);
        x = std::move(right.x);
        y = std::move(right.y);
        z = std::move(right.z);
        return *this;
    }

    // 随机初始化到 [-1.f, 1.f]
    void random_init() {
        for (int i = 0; i < x * y * z; ++i)
            data_ptr_[i] = dis(gen);
    }

    T& operator[] (size_t x, size_t y, size_t z = 0) {
        if (z * this->x * this->y + x * this->y + y >= this->z * this->x * this->y) {
            std::cout << "out of range" << std::endl;
        }
        return view_[z, x, y];
    }
    const T &get_const(size_t x, size_t y, size_t z = 0) const {
        return view_[z, x, y];
    }

    size_t extent(size_t index) {
        return view_.extent(index);
    }

    Matrix operator + (const Matrix & right) const {

        if (x != right.x || y != right.y || z != right.z) {
            throw std::runtime_error("Matrix::operator +");
        }

        Matrix ret(x, y, z);

        for (int i = 0; i < x; ++i) {
            for (int j = 0; j < y; ++j) {
                for (int k = 0; k < z; ++k) {
                    ret[i, j, k] = get_const(i, j, k) + right.get_const(i, j, k);
                }

            }
        }
        return ret;
    }

    Matrix operator - (const Matrix & right) const {

        if (x != right.x || y != right.y || z != right.z) {
            throw std::runtime_error("Matrix::operator -");
        }

        Matrix ret(x, y, z);

        for (int i = 0; i < x; ++i) {
            for (int j = 0; j < y; ++j) {
                for (int k = 0; k < z; ++k) {
                    ret[i, j, k] = get_const(i, j, k) - right.get_const(i, j, k);
                }

            }
        }
        return ret;
    }

    Matrix operator * (const Matrix & right) const {
        if (y != right.x || z != right.z)
            throw std::runtime_error("can't multiply matrix");

        Matrix ret(x, right.y, z);
        for (int k = 0;k < z; ++k) {

            for (int i = 0; i < x; i++) {
                for (int j = 0; j < right.y; j++) {
                    ret[i, j, k] = 0;
                    for (int t = 0; t < y; t++)
                        ret[i ,j, k] += get_const(i, t, k) * right.get_const(t, j, k);
                }
            }

        }
        return ret;
    }

    Matrix& operator += (const Matrix & right) {
        if (x != right.x || y != right.y || z != right.z) {
            throw std::runtime_error("Matrix::operator +");
        }
        for (int i = 0; i < x; ++i) {
            for (int j = 0; j < y; ++j) {
                for (int k = 0; k < z; ++k) {
                    operator[](i, j, k) = get_const(i, j, k) + right.get_const(i, j, k);
                }
            }
        }
        return *this;
    }
    Matrix& operator -= (const Matrix & right) {
        if (x != right.x || y != right.y || z != right.z) {
            throw std::runtime_error("Matrix::operator -");
        }
        for (int i = 0; i < x; ++i) {
            for (int j = 0; j < y; ++j) {
                for (int k = 0; k < z; ++k) {
                    operator[](i, j, k) = get_const(i, j, k) - right.get_const(i, j, k);
                }
            }
        }
        return *this;
    }

    Matrix& operator *= (const Matrix & right) {
        if (y != right.x || z != right.z)
            throw std::runtime_error("can't multiply matrix");

        Matrix ret(x, right.y, z);
        for (int k = 0;k < z; ++k) {

            for (int i = 0; i < x; i++) {
                for (int j = 0; j < right.y; j++) {
                    ret[i, j, k] = 0;
                    for (int t = 0; t < y; t++)
                        ret[i ,j, k] += get_const(i, t, k) * right.get_const(t, j, k);
                }
            }

        }
        *this = std::move(ret);
        return *this;
    }
};

NAMESPACE_END(nl)
