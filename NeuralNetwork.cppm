/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/06 22:51
*******************************************************************************/

module;
#include <iostream>
#include <array>
#include <memory>
#include <numeric>
#include <vector>
#include <random>
#include "tools.h"

export module NeuralNetwork;

import Matrix;

export
NAMESPACE_BEGIN(nl)

// ======================================= 激活函数 、sigmoid
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



// ======================================= 损失函数、均方误差
struct LossFunction {
    virtual double action(double, double) = 0;
    virtual double deaction(double, double) = 0;

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



// ========================================层、隐藏层、输出层
struct Layout {
    double speed = 0.01;
    std::vector<double> in{}, out{}, next_in{}, gradient{}; // gradient 中是保存的供前一层使用的next梯度
    nl::Matrix<double> weight{};
    std::shared_ptr<Activation> action{};

    Layout(size_t size, std::shared_ptr<Activation> p)
            : out(std::vector<double>(size)), gradient(std::vector<double>(size)) , action(p) {  }

    Layout(Layout &&layout) {
        in = std::move(layout.in);
        out = std::move(layout.out);

        weight = std::move(layout.weight);
        gradient = std::move(layout.gradient);
        action = std::move(layout.action);
    }
    size_t size() {
        return out.size();
    }
    void forward(const std::vector<double> &in) {
        this->in = in;
        for (int i = 0;i < in.size(); ++i) {
            out[i] = action->action(in[i]);
        }

        // 如果当前是最后一层，直接结束，返回值不需要
        if (next_in.empty()) {
            return ;
        }

        for (int i = 0;i < next_in.size(); ++i) {
            double sum{};
            for (int j = 0;j < out.size(); ++j) {
                sum += out[j] * weight[i, j];
            }
            next_in[i] = sum;
        }
    }
    virtual void backward(const std::vector<double>&) = 0;

    virtual ~Layout () = default;
};

struct HindLayout : Layout {
    HindLayout(size_t size, std::shared_ptr<Activation> p = std::make_shared<Sigmoid>()) : Layout(size, p) {  }

    HindLayout(HindLayout &&layout) : Layout(std::move(layout)) {  }

    // 修改权值
    double update_weight(double old_weight, double speed, double gradient) {
        return old_weight - speed * gradient;
    }

    // 反向传播
    void backward(const std::vector<double> &next_gradient) {

        // 保存梯度让下一层使用
        for (int i = 0;i < out.size(); ++i) {
            double sum{};
            for (int j = 0;j < next_gradient.size(); ++j) {
                sum += next_gradient[j] * weight[j, i];
            }
            gradient[i] = sum * action->deaction(in[i]);
        }

        // 修改权重，使用的是上一层保存的梯度 * out[i]
        for (int i = 0;i < weight.x; ++i) {
            for (int j = 0;j < weight.y; ++j) {
                weight[i, j] = update_weight(weight[i, j], speed, next_gradient[i] * out[j]);

            }
        }

    }


};


struct OutLayout : Layout {
    std::shared_ptr<LossFunction> loss_function = std::make_shared<MeanSquaredError>();


    OutLayout(size_t size,std::shared_ptr<Activation> p = std::make_shared<Sigmoid>()) : Layout(size, p) {  }
    OutLayout(HindLayout &&layout) : Layout(std::move(layout)) {  }

    // 输出层的反向传播，传入参数为真实结果
    void backward(const std::vector<double> &true_value) override {
        for (int i = 0;i < gradient.size(); ++i) {
            gradient[i] = loss_function->deaction(out[i], true_value[i]) * action->deaction(in[i]);
        }
    }
    ~OutLayout() override = default;

};


struct Backpropagation {
public:
    std::vector<std::shared_ptr<Layout>> layouts;

    template <typename ... Args>
    Backpropagation(Args&& ...args) requires (std::is_same_v<Args,int> && ...) && (sizeof ... (Args) >= 2) {
        std::vector<int> arg = {args...};

        for (int i = 0;i < arg.size() - 1; ++i) {
            add_layout<HindLayout>(arg[i]);
        }
        add_layout<OutLayout>(arg.back());

    }

    template <typename T>
    void add_layout(size_t size) {
        if (!layouts.empty()) {
            auto &prev_layout = layouts.back();
            size_t prev_size = prev_layout->size();
            prev_layout->next_in = std::vector<double>(size);
            prev_layout->weight = nl::Matrix<double>(size, prev_size);
            prev_layout->weight.random_init();
        }
        layouts.push_back(std::make_shared<T>(size));
    }

    void train(std::initializer_list<double> in, std::initializer_list<double> res) {

        layouts.front()->forward(in);
        // 前向传播
        for (int i = 1;i < layouts.size(); ++i) {
            layouts[i]->forward(layouts[i - 1]->next_in);
        }

        // 反向传播
        auto &layout = layouts.back();
        layout->backward(res);
        for (int i = layouts.size() - 2;i >= 0; --i) {
            layouts[i]->backward(layouts[i+1]->gradient);
        }

    }

    std::vector<double> forecast(std::initializer_list<double> in) {
        layouts.front()->forward(in);
        // 前向传播
        for (int i = 1;i < layouts.size(); ++i) {
            layouts[i]->forward(layouts[i - 1]->next_in);
        }

        return layouts.back()->out;
    }

};

NAMESPACE_END(nl)

