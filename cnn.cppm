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


Tensor<float, 2> Convolution(const Tensor<float, 2> &src, const Tensor<float, 2> &kernel, size_t stride = 1, size_t padding = 0) {
    int in_w = src.extent(1), kernel_w = kernel.extent(1);
    int in_h = src.extent(0), kernel_h = kernel.extent(0);
    int out_w = (in_w + 2 * padding - kernel_w) / stride + 1;
    int out_h = (in_h + 2 * padding - kernel_h) / stride + 1;

    Tensor<float, 2> ret(out_h, out_w);

    for (int i = 0; i < out_h; i++) {
        for (int j = 0; j < out_w; j++) {
            double sum{};

            for (int x = 0; x < kernel_h; x++) {
                for (int y = 0; y < kernel_w; y++) {
                    size_t in_x = i * stride + x;
                    size_t in_y = j * stride + y;
                    if (in_x >= in_h || in_y >= in_w)
                        continue;
                    sum += src[in_x, in_y] * kernel[x, y];
                }
            }
            ret[i, j] = sum;
        }
    }

    return ret;
}

struct CNNLayer {
    Tensor<float, 2> in, out, gradient;
    size_t in_size, out_size;

    virtual void forward(const Tensor<float, 2> &) = 0;
    virtual void backward(const Tensor<float, 2> &) = 0;
    virtual ~CNNLayer() = default;
};



// 卷积层
class ConvLayer : public CNNLayer {
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
    void forward(const Tensor<float, 2> &in) override {
        this->in = in;

    }
    void backward(const Tensor<float, 2> &) override {

    }


};

// 池化层
class PoolLayer : public CNNLayer {

public:

    // @TODO 应该提供不同的卷积操作
    void forward(const Tensor<float, 2> &) override {

    }
    void backward(const Tensor<float, 2> &) override {

    }

};

// 激活层
class ActionLayer : public CNNLayer {
    std::shared_ptr<Activation> action_ = std::make_shared<Relu>();
public:
    ActionLayer() = default;

    explicit ActionLayer(std::shared_ptr<Activation> action) : action_(std::move(action)) {  }

    void forward(const Tensor<float, 2> &in) override {
        this->in = in;
        this->out = in.copy();
        this->out.foreach([this](float &val) {
            val = action_->action(val);
        });

    }
    void backward(const Tensor<float, 2> &) override {

    }
};

// 全连接层
class FCLayer : public CNNLayer {
    BP bp;

public:
    // @TODO 权值展平,塞入PB
    void forward(const Tensor<float, 2> &) override {

    }
    void backward(const Tensor<float, 2> &) override {

    }

};

// 输入层
class InputLayer : public CNNLayer {

public:
    // @TODO 输入Tensor, 并进行归一化等操作
    void forward(const Tensor<float, 2> &) override {

    }
    void backward(const Tensor<float, 2> &) override {

    }
};

class OutputLayer : public CNNLayer {

public:
    // @TODO 输入Tensor, 并进行归一化等操作
    void forward(const Tensor<float, 2> &) override {

    }
    void backward(const Tensor<float, 2> &) override {

    }
};

export
class CNN {
    std::vector<std::shared_ptr<CNNLayer>> layouts_;

public:
    CNN() = default;

    void add_layer(std::shared_ptr<CNNLayer> &layer) {

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
