/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <fstream>
#include <iomanip>
#include <vector>
#include <memory>
#include <mdspan>
#include "tools.h"
export module Matrix;

import MultArray;

export
NAMESPACE_BEGIN(nl)



template <typename T, size_t ROW, size_t COL>
class Matrix {
    int row_{}, col_{};
    std::shared_ptr<T[]> ptr_;
    std::mdspan<T,std::extents<size_t,ROW,COL>> data_;
public:
    // 拥有所有权
    explicit Matrix(const std::vector<T> &datas) {
        int pos{};
        ptr_ = std::make_shared<T>(row_ * col_);
        data_ = std::mdspan(ptr_.get(), row_ , col_);
        for (int i = 0;i < ROW; ++i) {
            for (int j = 0;j < COL; ++j) {
                data_[i][j] = datas[pos++];
            }
        }
    }
    // 无所有权
    explicit Matrix(T *ptr) : ptr_(ptr) , data_(ptr, ROW, COL) {  }

    T& operator[](int row, int col) {
        return data_[row][col];
    }

    Matrix operator + (const Matrix & right) const {
        if (row_ != right.row_ || col_ != right.col_)
            throw std::runtime_error("Matrix addition failed");

        Matrix ret(row_, col_);
        for (int i = 0; i < row_; i++)
            for (int j = 0; j < col_; j++)
                ret.data_[i][j] = data_[i][j] + right.data_[i][j];

        return ret;
    }
    Matrix operator - (const Matrix & right) const {
        if (row_ != right.row_ || col_ != right.col_)
            throw std::runtime_error("Matrix addition failed");

        Matrix ret(row_,col_);
        for (int i = 0; i < row_; i++)
            for (int j = 0; j < col_; j++)
                ret.data_[i][j] = data_[i][j] + right.data_[i][j];
        return {};
    }
    Matrix operator * (const Matrix & right) const {
        if (col_ != right.row_)
            throw std::runtime_error("can't multiply matrix");

        Matrix ret(row_,right.col_);
        for (int i = 0; i < row_; i++) {
            for (int j = 0; j < right.col_; j++) {
                ret.data_[i][j] = 0;
                for (int k = 0; k < col_; k++)
                    ret.data_[i][j] += data_[i][k] * right.data_[k][j];
            }
        }
        return ret;
    }
    Matrix& operator += (const Matrix & right) {
        *this = *this + right;
        return *this;
    }
    Matrix& operator -= (const Matrix & right) {
        *this = *this - right;
        return *this;
    }
    Matrix& operator *= (const Matrix & right) {
        *this = *this * right;
        return *this;
    }
};

// template <typename T>
// class Matrix {
//     size_t row_{}, col_{};
//     std::shared_ptr<double[]> data_ptr_;
//     nl::MultArray<double> data_{};
//     friend std::ostream& operator << (std::ostream& os, const Matrix& matrix);
//     friend std::istream& operator >> (std::istream& is, Matrix& matrix);
// public:
//     Matrix(T *ptr, int row, int col) : data_(ptr,row,col) {
//     }
//
//     Matrix(const size_t row, const size_t col) :
//         row_(row), col_(col),
//         data_ptr_(std::make_shared<double[]>(col * row)),
//         data_(data_ptr_.get(), row, col) {  }
//
//     Matrix(const Matrix &other) = default;
//
//     Matrix(const std::vector<double> &numbers, int row, int col) {
//         if (numbers.size() < row * col)
//             throw std::runtime_error("vector size is too small");
//
//         data_ptr_ = std::make_shared<double[]>(col * row);
//         data_ = nl::MultArray<double>(data_ptr_.get(), row, col);
//         row_ = row;
//         col_ = col;
//
//         int index{};
//         for (int i = 0;i < row; i++)
//             for (int j = 0;j < col; j++)
//                 data_[i][j] = numbers[index ++];
//     }
//
//     Matrix& operator = (const Matrix &right) {
//         new (this) Matrix(right);
//         return *this;
//     }
//
//     Matrix& operator += (const Matrix & right) {
//         *this = *this + right;
//         return *this;
//     }
//     Matrix& operator -= (const Matrix & right) {
//         *this = *this - right;
//         return *this;
//     }
//     Matrix& operator *= (const Matrix & right) {
//         *this = *this * right;
//         return *this;
//     }
// };
//
// std::ostream& operator << (std::ostream& os, const Matrix& matrix) {
//     for (int i = 0; i < matrix.row_;i++) {
//         for (int j = 0; j < matrix.col_;j++) {
//             os << std::fixed << std::setprecision(6) << matrix.data_[i][j] << ' ';
//         }
//         os << '\n';
//     }
//     os << '\n';
//     return os;
// }
// std::istream &operator >> (std::istream &is, Matrix &matrix) {
//     std::vector<double> datas;
//     std::string line;
//     int row{}, col{};
//     while (std::getline(is, line) && line != "\n" && line != "\r\n") {
//         std::istringstream iss(line);
//         double value{};
//         col = 0;
//         while (iss >> value) {
//             datas.emplace_back(value);
//             col ++;
//         }
//         row ++;
//     }
//     matrix = Matrix(datas, row, col);
//     return is;
// }

// Matrix 有两种构造方式
// 一种自己拥有所有权
// 一种获得view

// template <typename T,size_t ROW, size_t COL>
// struct MatrixSize : std::extents<T,ROW,COL> {  };
//
// template <typename T,size_t ROW, size_t COL>
// class Matrix {
//     std::mdspan<T, MatrixSize<T,ROW,COL>> data_{};
// public:
//     Matrix(T *data,int row, int col) : data_(data,row,col) {  }
//     T& operator[](int i,int j) {
//         return data_[i][j];
//     }
//
//     Matrix operator + (Matrix right) {
//
//         return {};
//     }
//
// };

NAMESPACE_END(nl)
