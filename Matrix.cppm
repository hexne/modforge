/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <fstream>
#include <sstream>
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
    explicit Matrix(const std::vector<T> &datas,const int row, const int col) : Matrix(row, col) {
        int pos{};
        for (int i = 0;i < ROW; ++i) {
            for (int j = 0;j < COL; ++j) {
                data_[i][j] = datas[pos++];
            }
        }
    }
    // 无所有权
    explicit Matrix(T *ptr) : row_(ROW), col_(COL), data_(ptr, ROW, COL) {  }

    Matrix(const int row, const int col) : row_(row), col_(col) {
        ptr_ = std::make_shared<T>(row_ * col_);
        data_ = std::mdspan(ptr_.get(), row_ , col_);
    }

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
    // @TODO, += 应该直接作用于left上 ，而不是创建临时对象再赋值
    // -=, *= 同理
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

template <typename T, size_t ROW, size_t COL>
std::ostream& operator << (std::ostream& os, const Matrix<T, ROW, COL>& matrix) {
    for (int i = 0; i < matrix.row_;i++) {
        for (int j = 0; j < matrix.col_;j++) {
            os << std::fixed << std::setprecision(6) << matrix.data_[i][j] << ' ';
        }
        os << '\n';
    }
    os << '\n';
    return os;
}

template <typename T, size_t ROW, size_t COL>
std::istream &operator >> (std::istream &is, Matrix<T, ROW, COL> &matrix) {
    std::vector<double> datas;
    std::string line;
    int row{}, col{};
    while (std::getline(is, line) && line != "\n" && line != "\r\n") {
        std::istringstream iss(line);
        double value{};
        col = 0;
        while (iss >> value) {
            datas.emplace_back(value);
            col ++;
        }
        row ++;
    }
    matrix = Matrix<T,ROW,COL> (datas, row, col);
    return is;
}

NAMESPACE_END(nl)
