/*******************************************************************************
 * @Author : hexne
 * @Data   : 2025/03/26 22:21
*******************************************************************************/

module;
#include <assert.h>
#include <cmath>
#include <ranges>
#include <random>
export module modforge.deep_learning.tools;

import modforge.tensor;

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

    ~MeanSquaredError() override = default;
};


/****************************** 优化器 ******************************/
class Optimizer {
public:
    virtual double get_speed(int cur, int count) = 0;
    virtual ~Optimizer() = default;
};

class CosineAnnealing : public Optimizer {
    double max_speed;
    double min_speed;
    int T_max;

public:
    CosineAnnealing(double max_speed, double min_speed, int T_max)
        : max_speed(max_speed), min_speed(min_speed), T_max(T_max) {}

    double get_speed(int cur, int count) override {
        // 计算当前进度比例 (0-1)
        double progress = static_cast<double>(cur % T_max) / T_max;

        // 应用余弦退火公式
        return min_speed + 0.5 * (max_speed - min_speed) *
               (1 + std::cos(progress * M_PI));
    }
};


/**************************************** 归一化 ****************************************/
struct Normalization {
    virtual double action(double num) = 0;
    virtual double deaction(double num) = 0;
    virtual ~Normalization() = default;
};

/****************************** 获取随机值 ******************************/
double GetRandom(const double min, const double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}
template<typename T, size_t Extent = 2>
void random_tensor(Tensor<T, Extent>tensor, double min, double max) {
    tensor.foreach([&](T &val) {
        val = GetRandom(min, max);
    });

}

namespace OneHot {
    // type 从0开始
    int out_to_type(const Vector<float> &out) {
        int pos = 0;
        float max = out[0];
        for (int i = 1; i < out.size(); ++i) {
            if (out[i] > max) {
                max = out[i];
                pos = i;
            }
        }
        return pos;
    }

    template<size_t N>
    Vector<float> type_to_onehot(int type) {
        Vector<float> ret(N);
        for (int i = 0; i < N; ++i)
            ret[i] = 0.f;
        ret[type] = 1;
        return ret;
    }

}


}   // export {