/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

module;
#include <fstream>
#include <iomanip>
#include <vector>
#include <memory>
#include "tools.h"
export module Matrix;

import MultArray;

export
NAMESPACE_BEGIN(nl)

class Matrix {
    size_t row_{}, col_{};
    std::shared_ptr<double[]> data_ptr_;
    nl::MultArray<double> data_{};
    friend std::ostream& operator<<(std::ostream& os, const Matrix& matrix);
public:
    static Matrix LoadMatrix(const std::string &file_name) {

        std::ifstream in(file_name);
        if (std::ifstream in(file_name); !in.is_open())
            throw std::runtime_error("open file failed");
        std::string line;
        std::getline(in, line);
        int col{};
        for (int i = 0;i < line.size(); ++i) {
            while (line[i] == ' ')
                ;
            col ++;
        }
        while (std::getline(in, line)) {
            // @TODO

        }



        return {};
    }
    static Matrix LoadMatrix(const std::string &file_name,int row, int col) {
        std::ifstream in(file_name);
        if (!in.is_open())
            throw std::runtime_error("open file failed");

        Matrix ret(row,col);
        for (int i = 0;i < row;i++)
            for (int j = 0;j < col;j++)
                in >> ret.data_[i][j];

        return ret;
    }
    static Matrix LoadMatrix(const std::vector<double> &numbers, int row, int col) {
        if (numbers.size() < row * col)
            throw std::runtime_error("vector size is too small");

        Matrix ret(row,col);
        int index{};
        for (int i = 0;i < row; i++)
            for (int j = 0;j < col; j++)
                ret.data_[i][j] = numbers[index ++];
        return ret;
    }

    Matrix() = default;
    Matrix(size_t row, size_t col) :
        row_(row), col_(col),
        data_ptr_(std::make_shared<double[]>(col * row)),
        data_(data_ptr_.get(), row, col) {  }
    Matrix(const Matrix &other) {
        row_ = other.row_;
        col_ = other.col_;
        data_ptr_ = other.data_ptr_;
        data_ = other.data_;
    }
    Matrix& operator = (const Matrix &other) {
        new (this) Matrix(other);
        return *this;
    }

    Matrix operator + (const Matrix & right) const {
        if (row_ != right.row_ || col_ != right.col_)
            throw std::runtime_error("Matrix addition failed");

        Matrix ret(row_,col_);
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

        return *this;
    }
    Matrix& operator *= (const Matrix & right) {
        *this = *this * right;
        return *this;
    }
};

std::ostream& operator << (std::ostream& os, const Matrix& matrix) {
    os << "[\n";

    for (int i = 0; i < matrix.row_;i++) {
        for (int j = 0; j < matrix.col_;j++) {
            os << std::setw(5) << matrix.data_[i][j] << ' ';
        }
        os << '\n';
    }
    os << "]\n";
    return os;
}

NAMESPACE_END(nl)
