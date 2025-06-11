/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <memory>
#include <vector>
export module modforge.deep_learning.cnn;


import modforge.tensor;

import modforge.deep_learning.bp;
import modforge.deep_learning.tools;


struct Layer {
    Tensor<float, 3> in, out;

    virtual void forward(const Tensor<float, 3> &) = 0;
    virtual void backward(const Tensor<float, 2> &) = 0;
    virtual ~Layer() = default;
};



// 卷积层
class ConvLayer : public Layer {
    std::vector<Tensor<float, 2>> kernels_;
public:
    explicit ConvLayer(auto && ... sizes) requires (sizeof ...(sizes) >= 1){
        std::initializer_list<int> args {sizes ...};
        for (auto size : args) {
            Tensor<float, 2> tmp(size, size);
            random_tensor(tmp, -1, 1);
            kernels_.emplace_back(tmp);
        }
    }
    void forward(const Tensor<float, 3> &in) override {

    }

};

// 池化层
class PoolLayer : public Layer {

public:

    // @TODO 应该提供不同的卷积操作
    void forward(const Tensor<float, 3> &) override {

    }
};

// 激活层
class ActionLayer : public Layer {
    std::shared_ptr<Activation> action_ = std::make_shared<Relu>();
public:
    ActionLayer() = default;

    explicit ActionLayer(std::shared_ptr<Activation> action) : action_(std::move(action)) {  }

    void forward(const Tensor<float, 3> &in) override {
    }
};

// 全连接层
class FCLayer : public Layer {
    BP bp;

public:
    // @TODO 权值展平,塞入PB
    void forward(const Tensor<float, 3> &) override {

    }

};

// 输入层
class InputLayer : public Layer {

public:
    // @TODO 输入Tensor, 并进行归一化等操作
    void forward(const Tensor<float, 3> &) override {

    }
};

class OutputLayer : public Layer {

public:
    // @TODO 输入Tensor, 并进行归一化等操作
    void forward(const Tensor<float, 3> &) override {

    }
    void backward(const Tensor<float, 2> &) override {

    }
};

export
class CNN {
    std::vector<std::shared_ptr<Layer>> layouts_;

public:
    CNN() = default;

    void add_layer(std::shared_ptr<Layer> &layer) {

    }

    void forward(const Tensor<float, 2> &in) {

    }
    void backward(const Vector<float> &out) {

    }


    // CNN的输入是张量，二维或者三维, 输出是onehot
    void train(const Tensor<float, 2> &in, const Vector<float> &out) {
        forward(in);
        backward(out); }

    void train(const std::vector<std::pair<Vector<float>, Vector<float>>> &dataset,
                float train_proportion, size_t train_count, int seed, bool updata_acc = false) {


    }
    const Vector<float> forcast(const Tensor<float, 2> &in) {
        forward(in);
        // @TODO 输出Vector应该被适配
        return {};
    }
};
