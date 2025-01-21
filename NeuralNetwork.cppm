/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/06 22:51
*******************************************************************************/

module;
#include <array>
#include <iostream>
#include <memory>
#include <ostream>
#include <random>
#include <tuple>
#include <type_traits>

#include "tools.h"
export module NeuralNetwork;

// @TODO
constexpr double speed = 0.1;

std::random_device random_device;
std::mt19937 random_engine(random_device());
std::uniform_real_distribution<> distrib(0,1);


template <std::size_t... N>
struct NeuralNetworkHelper;

template <std::size_t Row, std::size_t Col, std::size_t... Args>
struct NeuralNetworkHelper<Row, Col, Args...> {
    using type = std::remove_cvref_t<decltype(std::tuple_cat(
                        std::declval<std::tuple<std::array<std::array<double,Row>, Col>>>(),
                        std::declval<typename NeuralNetworkHelper<Col, Args...>::type>()
                    ))>;
};

template <std::size_t Last>
struct NeuralNetworkHelper<Last> {
    using type = std::tuple<>;
};


struct Activate {
    virtual double activate(double) = 0;
    virtual double derivative(double) = 0;
    virtual ~Activate() = default;
};
struct NoneActivate : Activate {
    double activate(const double x) override {
        return x;
    }
    double derivative(const double x) override {
        return 1;
    }
};
struct Sigmoid : Activate {
    double activate(const double x) override {
        return 1.0 / (1 + exp(-x));
    }
    double derivative(const double x) override {
        return x * (1 - x);
    }
};
struct Tanh : Activate {
    double activate(const double x) override {
        return tanh(x);
    }
    double derivative(const double x) override {
        return 1.0 - x * x;  // tanh 的导数是 1 - tanh(x)^2
    }
};

struct LossFunction {
    // 损失函数对预测值求偏导
    virtual double derivative(double,double) = 0;
    virtual ~LossFunction() = default;
};

// 均方误差 损失函数
struct MeanSquaredError : LossFunction {
    double derivative(const double predict, const double real) override {
        return predict - real;
    }
};


export
NAMESPACE_BEGIN(nl)

template <std::size_t... N>
class NeuralNetwork {
    typename NeuralNetworkHelper<N ...>::type weights_;         // 权值
    typename NeuralNetworkHelper<N ...>::type gradients_;       //
    std::tuple<std::array<double, N> ...> gradients_line_;       //
    std::tuple<std::array<double, N> ...> inputs_;              // 每层的输入
    std::tuple<std::array<double, N> ...> outputs_;             // 每层的输出
    std::size_t layout_count_ = sizeof ... (N);
    enum class ActivateType {
        Sigmoid,
        Tanh,
    };
    enum class LossFunctionType {
        MeanSquaredError,
    };
    std::shared_ptr<Activate> activate_function_ {new Sigmoid};
    std::shared_ptr<Activate> out_activate_function_ {new NoneActivate};
    std::shared_ptr<LossFunction> loss_function_ {new MeanSquaredError};

    template <std::size_t index>
    static constexpr size_t get_neural_size() {
        constexpr static size_t sizes[] = { N ... };
        return sizes[index];
    }

    template <size_t index = 0>
    void init_weight() {
        if constexpr (index < sizeof... (N) - 1) {
            auto& weights = std::get<index>(weights_);

            for (int i = 0;i < weights.size(); ++i) {
                for (int j = 0;j < weights[i].size(); ++j) {
                    weights[i][j] = distrib(random_engine);
                }
            }
            init_weight<index + 1>();
        }
    }

    static size_t constexpr get_input_count() {
        return get_neural_size<0>();
    }

    static size_t constexpr get_output_count() {
        return get_neural_size<sizeof...(N) - 1>();
    }
public:

    NeuralNetwork() {
        init_weight();
    }

    std::array<double,get_output_count()> forward(const std::array<double,get_input_count()> &input) {
        auto &tmp = std::get<0>(outputs_);
        tmp = input;
        forward();
        return std::get<sizeof...(N) - 1>(outputs_);
    }


    template <size_t index = 0>
    void forward() {
        if constexpr (index < sizeof ...(N) - 1) {
            auto& weight = std::get<index>(weights_);
            auto& input_cur = std::get<index>(inputs_);
            auto& input_next = std::get<index + 1>(inputs_);
            auto& output = std::get<index>(outputs_);

            for (int i = 0;i < input_cur.size(); ++i)
                output[i] = activate_function_->activate(input_cur[i]);

            for (int i = 0;i < weight.size(); ++i) {
                input_next[i] = 0;
                for (int j = 0;j < weight[i].size(); ++j) {
                    input_next[i] += weight[i][j] * output[j];
                }
            }
            forward<index + 1>();
        }
        else if constexpr (index == sizeof...(N) - 1) {
            auto &input = std::get<index>(inputs_);
            auto &output = std::get<index>(outputs_);

            for (int i = 0;i < input.size(); ++i)
                output[i] = out_activate_function_->activate(input[i]);
        }

    }

    template <size_t index = sizeof ... (N) - 2>
    void backward() {
        if constexpr (index > 0) {
            // 隐藏层
            auto& input = std::get<index - 1>(inputs_);
            auto& output = std::get<index - 1>(outputs_);
            auto& weight = std::get<index - 1>(weights_);
            auto& gradient = std::get<index - 1>(gradients_);
            auto& next_dZ = std::get<index + 1>(gradients_line_);

            // 计算当前层的激活值梯度
            std::array<double, get_neural_size<index>()> dA;
            for (size_t i = 0; i < output.size(); ++i) {
                for (size_t j = 0; j < weight[i].size(); ++j) {
                    dA[i] += next_dZ[j] * weight[j][i];
                }
            }

            // 计算当前层的线性部分梯度
            std::array<double, get_neural_size<index>()> dZ;
            for (size_t i = 0; i < output.size(); ++i) {
                dZ[i] = dA[i] * activate_function_->derivative(output[i]);
            }

            // 计算权重梯度
            for (size_t i = 0; i < weight.size(); ++i) {
                for (size_t j = 0; j < weight[i].size(); ++j) {
                    gradient[i][j] = dZ[i] * input[j];
                }
            }

            // 更新权重
            for (size_t i = 0; i < weight.size(); ++i) {
                for (size_t j = 0; j < weight[i].size(); ++j) {
                    weight[i][j] -= speed * gradient[i][j];
                }
            }

            // 保存当前层的梯度
            std::get<index>(gradients_line_) = dZ;

            backward<index - 1>();
        }
    }
    void backward(const std::array<double, get_output_count()> &result) {
        constexpr size_t index = sizeof...(N) - 1;
        // 输出层
        auto& output = std::get<index>(outputs_);
        auto& input = std::get<index - 1>(inputs_);
        auto& weight = std::get<index - 1>(weights_);
        auto& gradient = std::get<index - 1>(gradients_);

        // 计算输出层的激活值梯度
        // aL中为计算的差值
        std::array<double, get_neural_size<index>()> dAL;
        for (size_t i = 0; i < output.size(); ++i) {
            dAL[i] = loss_function_->derivative(output[i], result[i]);
        }

        // 计算输出层的线性部分梯度
        std::array<double, get_neural_size<index>()> dZ;
        for (size_t i = 0; i < output.size(); ++i) {
            dZ[i] = dAL[i] * out_activate_function_->derivative(output[i]);
        }

        // 计算权重梯度
        for (size_t i = 0; i < weight.size(); ++i) {
            for (size_t j = 0; j < weight[i].size(); ++j) {
                gradient[i][j] = dZ[i] * input[j];
            }
        }

        // 更新权重
        for (size_t i = 0; i < weight.size(); ++i) {
            for (size_t j = 0; j < weight[i].size(); ++j) {
                weight[i][j] -= speed * gradient[i][j];
            }
        }

        // 保存输出层的梯度
        std::get<index>(gradients_line_) = dZ;
        backward();
    }

    void set_activate(ActivateType activate) {
        if (activate == ActivateType::Sigmoid)
            activate_function_ = std::make_shared<Sigmoid>();
        else if (activate == ActivateType::Tanh)
            activate_function_ = std::make_shared<Tanh>();
        else
            ;

    }
    void set_out_activate(ActivateType activate) {
        if (activate == ActivateType::Sigmoid)
            out_activate_function_ = std::make_shared<Sigmoid>();
        else if (activate == ActivateType::Tanh)
            out_activate_function_ = std::make_shared<Tanh>();
        else
            ;
    }
    void set_loss_function(LossFunctionType loss_function) {
        if (loss_function == LossFunctionType::MeanSquaredError)
            loss_function_ = std::make_shared<MeanSquaredError>();
        else
            ;
    }

};

NAMESPACE_END(nl)

