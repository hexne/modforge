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
constexpr double speed = 0.001;

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
struct Sigmoid : Activate {
    double activate(const double x) override {
        return 1.0 / (1 + exp(-x));
    }
    double derivative(const double x) override {
        return x * (1 - x);
    }
};
struct Tanh : Activate {
    double activate(const double) override {
        // @TODO
        return 0.0;
    }
    double derivative(const double x) override {
        return {};
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

// 对于传入的参数 <2,3,4,2>
// 对应的矩阵为
// 2 x 3, 3 x 4, 4 x 2, 而非常规认为的 3 x 2, 4 x 3, 2 x 4
template <std::size_t... N>
class NeuralNetwork {
    typename NeuralNetworkHelper<N ...>::type weights_;         // 权值
    typename NeuralNetworkHelper<N ...>::type tmp_value_;       // ?
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
    std::shared_ptr<Activate> out_activate_function_ {new Sigmoid};
    std::shared_ptr<LossFunction> loss_function_ {new MeanSquaredError};

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
        return get_input_count_impl<N...>();
    }

    template <size_t First, size_t ... Last>
    static size_t constexpr get_input_count_impl() {
        return First;
    }

    static size_t constexpr get_output_count() {
        return get_output_count_impl<N...>();
    }
    template <std::size_t First, std::size_t Last, std::size_t ... Args>
    static size_t constexpr get_output_count_impl() {
        return get_output_count_impl<Last, Args...>();
    }
    template <std::size_t Last>
    static size_t constexpr get_output_count_impl() {
        return Last;
    }
public:


    template <size_t index = 0>
    void print() {
        if constexpr (index < sizeof ...(N) - 1) {
            auto& weights = std::get<index>(weights_);

            std::cout << "[\n";
            for (int i = 0;i < weights.size(); ++i) {
                for (int j = 0;j < weights[i].size(); ++j) {
                    std::cout << weights[i][j] << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "]\n";
            print<index + 1>();
        }


    }
    NeuralNetwork() {
        init_weight<0>();
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

    template <size_t index = sizeof ... (N) - 1>
    void backward() {
        // 如果是最后一层
        if constexpr (index == sizeof...(N) - 1) {


            backward<index - 1>();
        }
        // 如果是中间层
        else if constexpr (index > 0) {



            backward<index - 1>();
        }

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

