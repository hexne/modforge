/*******************************************************************************
 * @Author : hexne
 * @Data   : 2025/03/26 22:21
*******************************************************************************/

module;
#include <cmath>
#include <ranges>
#include <random>
export module modforge.deep_learning.tools;

import modforge.matrix;

export {

/**************************************** 激活函数 ****************************************/
struct Activation {
    virtual double action(double num) = 0;
    virtual double deaction(double num) = 0;
    virtual ~Activation() = default;
};

struct Sigmoid : Activation {
    double action(double num) override {
        return 1.0 / (1.0 + exp(-num));
    }

    double deaction(double num) override {
        double sigmod = action(num);
        return sigmod * (1.0 - sigmod);
    }
    ~Sigmoid() override = default;
};

struct Relu : Activation {

    double action(double num) override {
        return std::max(0.0, num);
    }

    double deaction(double num) override {
        if (num > 0)
            return 1;
        return 0;
    }

    ~Relu() override = default;
};



/**************************************** 损失函数 ****************************************/
struct LossFunction {
    virtual double action(double predicted_value, double true_value) = 0;
    virtual double deaction(double predicted_value, double true_value) = 0;

    // virtual Matrix<float> action(Matrix<float> &predicted_value, Matrix<float> &true_value) = 0;
    // virtual Matrix<float> deaction(Matrix<float> &predicted_value, Matrix<float> &true_value) = 0;

    virtual ~LossFunction() = default;
};

struct MeanSquaredError : LossFunction {
    // 参数分别是 预测值 ， 真实值
    double action(double predicted_value, double true_value) override {
        return 0.5 * std::pow(true_value - predicted_value, 2);
    }
    double deaction(double predicted_value, double true_value) override {
        return predicted_value - true_value;
    }

    // Matrix<float> action(Matrix<float> &predicted_value, Matrix<float> &true_value) override {
    //     Matrix<float> result = predicted_value;
    //     for (int i = 0;i < result.y; ++i)
    //         result[0, i] = action(result[0, i], true_value[0, i]);
    //     return result;
    // }
    // Matrix<float> deaction(Matrix<float> &predicted_value, Matrix<float> &true_value) override {
    //     Matrix<float> result = predicted_value;
    //     for (int i = 0;i < result.y; ++i)
    //         result[0, i] = deaction(result[0, i], true_value[0, i]);
    //     return result;
    // }


    ~MeanSquaredError() override = default;
};



/**************************************** 归一化 ****************************************/
struct Normalization {
    virtual double action(double num) = 0;
    virtual double deaction(double num) = 0;
    virtual ~Normalization() = default;
};


double GetRandom(const double min, const double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}


template <size_t N>
struct OneHot {

    static std::array<bool,N> to_onehot(int type) {
        std::array<bool,N> ret{};
        ret[type - 1] = true;
        return ret;
    }

    static int to_type(std::array<bool,N> one_hot) {
        return std::ranges::find(one_hot,true) - one_hot.begin() + 1;
    }

};


}   // export {