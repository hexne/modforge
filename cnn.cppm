/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <cassert>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <utility>
#include <vector>
export module modforge.deep_learning.cnn;

import modforge.console;
import modforge.tensor;
import modforge.deep_learning.tools;

export class CNN;

// 标准卷积核
using Kernels = Tensor<float, 4>;
using FeatureMap = Tensor<float, 3>;
using FeatureExtent = struct FeatureExtent { int w{}, h{}, cannel{}; };
using PoolWindow = Tensor<float, 2>;

FeatureExtent get_out_extent(const FeatureExtent&in_extent, const Kernels &kernels, size_t stride, size_t padding) {

    FeatureExtent ret;

    ret.h = (in_extent.h + 2 * padding - kernels.extent(2)) / stride + 1;
    ret.w = (in_extent.w + 2 * padding - kernels.extent(3)) / stride + 1;
    ret.cannel = kernels.extent(0);

    return ret;
}

FeatureExtent get_out_extent(const FeatureExtent&in_extent, const PoolWindow &pool_window, size_t stride, size_t padding) {
    FeatureExtent ret;

    ret.h = (in_extent.h + 2 * padding - pool_window.extent(0)) / stride + 1;
    ret.w = (in_extent.w + 2 * padding - pool_window.extent(1)) / stride + 1;
    ret.cannel = in_extent.cannel;

    return ret;
}


struct CNNLayer {
    FeatureMap in, out, gradient;
    FeatureExtent in_extent, out_extent;

    virtual void forward(const FeatureMap&) = 0;
    virtual void backward(const FeatureMap&) = 0;
    virtual ~CNNLayer() = default;
};



// 卷积层
class ConvLayer : public CNNLayer {
    Kernels kernels_;
    size_t stride_, padding_;
public:
    ConvLayer(size_t n , size_t size, FeatureExtent in_extent, size_t stride = 1, size_t padding = 0)
        : stride_(stride), padding_(padding) {

        this->in_extent = in_extent;
        kernels_ = Kernels(n, in_extent.cannel, size, size);
        out_extent = get_out_extent(in_extent, kernels_, stride, padding);

        random_tensor(kernels_, -1, 1);
    }

    void forward(const FeatureMap &in) override {
        this->in = in;

        int kernel_z = kernels_.extent(1);
        int kernel_h = kernels_.extent(2);
        int kernel_w = kernels_.extent(3);

        // 输入通道数和卷积核通道数需要匹配
        assert(in_extent.cannel == kernel_z);


        FeatureMap res(out_extent.cannel, out_extent.h, out_extent.w);

        for (int z = 0; z < out_extent.cannel; ++z) {
            for (int x = 0; x < out_extent.h; ++x) {
                for (int y = 0; y < out_extent.w; ++y) {
                    float sum{};

                    const int h_start = x * stride_ - padding_;
                    const int w_start = y * stride_ - padding_;

                    for (int kz = 0; kz < kernel_z; ++kz) {
                        for (int kh = 0; kh < kernel_h; ++kh) {
                            for (int kw = 0; kw < kernel_w; ++kw) {

                                const int ih = h_start + kh;
                                const int iw = w_start + kw;
                                if ((ih < 0 || ih >= in_extent.h) || (iw < 0 || iw >= in_extent.w))
                                    continue;
                                sum += this->in[kz, ih, iw] * kernels_[z, kz, kh, kw];
                            }
                        }
                    }

                    res[z, x, y] = sum;
                }
            }
        }

        this->out = std::move(res);
    }

    void backward(const FeatureMap &next_gradient) override {
        // 1. 初始化梯度张量
        this->gradient = FeatureMap(in_extent.cannel, in_extent.h, in_extent.w);
        gradient.foreach([](float &val) { val = 0.f; });

        Kernels kernel_gradients(kernels_.extent(0), kernels_.extent(1),
                                kernels_.extent(2), kernels_.extent(3));
        kernel_gradients.foreach([](float &val) { val = 0.f; });

        const int kernel_z = kernels_.extent(1);
        const int kernel_h = kernels_.extent(2);
        const int kernel_w = kernels_.extent(3);

        // 2. 计算输入梯度和卷积核梯度
        for (int z = 0; z < out_extent.cannel; ++z) {          // 输出通道
            for (int x = 0; x < out_extent.h; ++x) {          // 输出高度
                for (int y = 0; y < out_extent.w; ++y) {       // 输出宽度
                    const float grad = next_gradient[z, x, y];

                    const int h_start = x * stride_ - padding_;
                    const int w_start = y * stride_ - padding_;

                    // 2.1 计算输入梯度
                    for (int kz = 0; kz < kernel_z; ++kz) {       // 输入通道
                        for (int kh = 0; kh < kernel_h; ++kh) {    // 核高度
                            for (int kw = 0; kw < kernel_w; ++kw) { // 核宽度
                                const int ih = h_start + kh;
                                const int iw = w_start + kw;

                                if (ih < 0 || ih >= in_extent.h || iw < 0 || iw >= in_extent.w)
                                    continue;

                                gradient[kz, ih, iw] += grad * kernels_[z, kz, kh, kw];
                            }
                        }
                    }

                    // 2.2 计算卷积核梯度
                    for (int kz = 0; kz < kernel_z; ++kz) {       // 输入通道
                        for (int kh = 0; kh < kernel_h; ++kh) {    // 核高度
                            for (int kw = 0; kw < kernel_w; ++kw) { // 核宽度
                                const int ih = h_start + kh;
                                const int iw = w_start + kw;

                                if (ih < 0 || ih >= in_extent.h || iw < 0 || iw >= in_extent.w)
                                    continue;

                                kernel_gradients[z, kz, kh, kw] += grad * in[kz, ih, iw];
                            }
                        }
                    }
                }
            }
        }

        // 3. 更新卷积核权重 (需要学习率参数)
        const float learning_rate = 0.01; // 应该作为类成员变量
        // 替换原来的 foreach 部分
        for (size_t z = 0; z < kernels_.extent(0); ++z) {          // 输出通道数
            for (size_t kz = 0; kz < kernels_.extent(1); ++kz) {   // 输入通道数
                for (size_t kh = 0; kh < kernels_.extent(2); ++kh) { // 核高度
                    for (size_t kw = 0; kw < kernels_.extent(3); ++kw) { // 核宽度
                        kernels_[z, kz, kh, kw] -= learning_rate * kernel_gradients[z, kz, kh, kw];
                    }
                }
            }
        }
    }
};

// 池化层
class PoolLayer : public CNNLayer {
    PoolWindow pool_window_;
    size_t stride;
public:
    PoolLayer(size_t window_size, const FeatureExtent &in_extent, size_t stride = 1, size_t padding = 0) {
        this->in_extent = in_extent;
        this->stride = stride;
        pool_window_ = PoolWindow(window_size, window_size);
        out_extent = get_out_extent(this->in_extent, pool_window_, stride, padding);
    }
    void forward(const FeatureMap &in) override {
        this->in = in;

        const int pool_h = pool_window_.extent(0);
        const int pool_w = pool_window_.extent(1);


        FeatureMap res(in_extent.cannel, out_extent.h, out_extent.w);

        for (int cur_cannel = 0; cur_cannel < in_extent.cannel; ++cur_cannel) {
            for (int oh = 0; oh < out_extent.h; ++oh) {
                for (int ow = 0; ow < out_extent.w; ++ow) {
                    float cur_max = std::numeric_limits<float>::lowest();

                    const int start_x = oh * stride;
                    const int start_y = ow * stride;

                    for (int ph = 0; ph < pool_h; ++ph) {
                        for (int pw = 0; pw < pool_w; ++pw) {
                            const int in_x = start_x + ph;
                            const int in_y = start_y + pw;

                            if ((in_x < 0 || in_x >= in_extent.h) || (in_y < 0 || in_y >= in_extent.w))
                                continue;
                            cur_max = std::max(cur_max, this->in[cur_cannel, in_x, in_y]);
                        }
                    }

                    res[cur_cannel, oh, ow] = cur_max;
                }
            }
        }

        this->out = res;
    }

    void backward(const FeatureMap &next_gradient) override {
        FeatureMap gradient(in_extent.cannel, in_extent.h, in_extent.w);
        // 必须初始化为 全0 矩阵，最大池化 反向传播时只有最大值处会被传递梯度
        gradient.foreach([](float &val) {
            val = 0.f;
        });

        for (int c = 0; c < out_extent.cannel; ++c) {
            for (int oh = 0; oh < out_extent.h; ++oh) {
                for (int ow = 0; ow < out_extent.w; ++ow) {

                    int max_h = -1, max_w = -1;
                    float max_val = std::numeric_limits<float>::lowest();

                    // 遍历池化窗口
                    for (int ph = 0; ph < pool_window_.extent(0); ++ph) {
                        for (int pw = 0; pw < pool_window_.extent(1); ++pw) {
                            int ih = oh * stride + ph;
                            int iw = ow * stride + pw;

                            if (ih < 0 || ih >= in_extent.h || iw < 0 || iw >= in_extent.w)
                                continue;

                            if (in[c, ih, iw] > max_val) {
                                max_val = in[c, ih, iw];
                                max_h = ih;
                                max_w = iw;
                            }
                        }
                    }

                    // 只在最大值位置回传梯度
                    if (max_h != -1 && max_w != -1) {
                        gradient[c, max_h, max_w] = next_gradient[c, oh, ow];
                    }
                }
            }
        }

        this->gradient = std::move(gradient);
    }
};

// 激活层
class ActionLayer : public CNNLayer {
    std::shared_ptr<Activation> action_ = std::make_shared<Relu>();
public:
    ActionLayer(const FeatureExtent& in_extent ,std::shared_ptr<Activation> action = std::make_shared<Relu>())
        : action_(std::move(action)) {

        this->in_extent = this->out_extent = in_extent;
    }

    void forward(const FeatureMap &in) override {
        this->in = in;
        this->out = this->in.copy();
        this->out.foreach([&](float &val) {
            val = action_->action(val);
        });
    }


    void backward(const FeatureMap &next_gradient) override {
        this->gradient = FeatureMap(in_extent.cannel, out_extent.h, out_extent.w);
        for (int z = 0; z < out_extent.cannel; ++z) {
            for (int x = 0; x < out_extent.w; ++x) {
                for (int y = 0; y < out_extent.h; ++y) {
                    this->gradient[z, x, y] = action_->deaction(in[z, x, y]) * next_gradient[z, x, y];
                }
            }
        }
    }
};




// 全连接层
class FCLayer : public CNNLayer {
    friend class CNN;
    Kernels weight_;
    Vector<float> fc_in_, fc_out_;

    std::shared_ptr<Activation> action_ = std::make_shared<Sigmoid>();

public:
    FCLayer(int type_count, const FeatureExtent &in_extent) {
        this->in_extent = in_extent;

        out_extent = FeatureExtent { .cannel = type_count, .w = 1, .h = 1 };

        weight_ = Kernels(out_extent.cannel, in_extent.cannel, in_extent.h, in_extent.w);
        fc_in_ = Vector<float>(out_extent.cannel);
        fc_out_ = Vector<float>(out_extent.cannel);

        random_tensor(weight_, 0, 1);

    }

    void forward(const FeatureMap &in) override {
        this->in = in;

        for (int cur_kernel = 0; cur_kernel < out_extent.cannel; ++cur_kernel) {
            float sum{};
            for (int z = 0; z < in_extent.cannel; ++z) {
                for (int x = 0; x < in_extent.w; ++x) {
                    for (int y = 0; y < in_extent.h; ++y) {
                        sum += weight_[cur_kernel, z, x, y] * in[z, x, y];
                    }
                }
            }
            fc_in_[cur_kernel] = sum;
            fc_out_[cur_kernel] = action_->action(sum);
        }

        // 对out进行softmax变换
        // 计算最大值（数值稳定性处理）
        float max_val = fc_out_[0];
        for (int i = 1; i < out_extent.cannel; ++i) {
            if (fc_out_[i] > max_val) {
                max_val = fc_out_[i];
            }
        }

        // 2. 计算指数和
        float exp_sum = 0.0f;
        for (int i = 0; i < out_extent.cannel; ++i)
            exp_sum += std::exp(fc_out_[i] - max_val); // 减去最大值防止溢出

        // 3. 转换为概率分布
        for (int i = 0; i < out_extent.cannel; ++i)
            fc_out_[i] = std::exp(fc_out_[i] - max_val) / exp_sum;
    }

    // 全链接层输出
    void backward(const Vector<float> &next_gradient) {
        Vector<float> vector_gradient = next_gradient;
        for (int i = 0; i < vector_gradient.size(); ++i) {
            vector_gradient[i] *= action_->deaction(fc_in_[i]);
            // 对于Sigmoid，应乘以 fc_out_[i] * (1 - fc_out_[i])
        }

        // 强制打印梯度（调试）
        std::cout << "FC Gradients: ";
        for (int i = 0;i < vector_gradient.size(); ++i) {
            std::cout << vector_gradient[i] << " ";
        }
        std::cout << "\n";

        // 更新权重（确保学习率不为零）
        const float lr = 0.01f; // 临时调大学习率
        for (int c_out = 0; c_out < out_extent.cannel; ++c_out) {
            for (int c_in = 0; c_in < in_extent.cannel; ++c_in) {
                for (int h = 0; h < in_extent.h; ++h) {
                    for (int w = 0; w < in_extent.w; ++w) {
                        weight_[c_out, c_in, h, w] -= lr * vector_gradient[c_out] * in[c_in, h, w];
                    }
                }
            }
        }

        // 计算输入梯度（确保sum不为零）
        FeatureMap gradient(in_extent.cannel, in_extent.h, in_extent.w);
        for (int c_in = 0; c_in < in_extent.cannel; ++c_in) {
            for (int h = 0; h < in_extent.h; ++h) {
                for (int w = 0; w < in_extent.w; ++w) {
                    float sum = 0.0f;
                    for (int c_out = 0; c_out < out_extent.cannel; ++c_out) {
                        sum += weight_[c_out, c_in, h, w] * vector_gradient[c_out];
                    }
                    gradient[c_in, h, w] = sum;
                }
            }
        }
        this->gradient = std::move(gradient);
    }
    void backward(const FeatureMap &next_gradient) override {  }

};



class CNN {
    std::vector<std::shared_ptr<CNNLayer>> layouts_;
    std::shared_ptr<LossFunction> loss_ = std::make_shared<MeanSquaredError>();
    std::vector<std::pair<FeatureMap,Vector<float>>> train_, test_;
    FeatureExtent in_extent;

    FeatureExtent get_cur_in_extent() const {
        FeatureExtent ret;
        if (layouts_.empty()) {
            ret = in_extent;
        }
        else {
            ret = layouts_.back()->out_extent;
        }

        return ret;
    }
public:
    // 构造的时候就创建第一层，用于确定输入和输出的尺寸
    CNN(int w, int h, int cannel) {
        this->in_extent = FeatureExtent{ .w = w, .h = h, .cannel = cannel };
    }

    void add_conv_layer(size_t n, size_t size, size_t stride = 1, size_t padding = 0) {
        auto layer = std::make_shared<ConvLayer>(n, size, get_cur_in_extent(), stride, padding);
        layouts_.emplace_back(layer);
    }

    void add_action_layer(std::shared_ptr<Activation> action = std::make_shared<Relu>()) {
        auto layer = std::make_shared<ActionLayer>(get_cur_in_extent(), action);
        layouts_.emplace_back(layer);
    }

    void add_pool_layer(size_t window_size) {
        auto layer = std::make_shared<PoolLayer>(window_size, get_cur_in_extent());
        layouts_.emplace_back(layer);
    }

    void add_fc_layer(size_t type_count) {
        auto layer = std::make_shared<FCLayer>(type_count, get_cur_in_extent());
        layouts_.emplace_back(layer);
    }

    void forward(const FeatureMap &in) {
        for (auto &layer : layouts_) {
            layer->forward(in);
        }
    }
    void backward(const Vector<float> &res) {
        auto back_layer = dynamic_cast<FCLayer *>(layouts_.back().get());
        auto out = back_layer->fc_out_;

        Vector<float> in(res.size());
        for (int i = 0;i < res.size(); ++i) {
            in[i] = loss_->deaction(res[i], out[i]);
        }
        back_layer->backward(in);
        for (int i = layouts_.size() - 2; i >= 0; --i) {
            layouts_[i]->backward(layouts_[i+1]->gradient);
        }
    }


    void train(const FeatureMap &in, const Vector<float> &out) {
        forward(in);
        backward(out);
    }

    void train(std::vector<std::pair<FeatureMap, Vector<float>>> &dataset,
                float train_proportion, size_t train_count, int seed, bool updata_acc = false) {

        Console::hind_cursor();

        std::cout << "处理数据集" << std::endl;
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> dis(0, dataset.size());
        for (int i = 0;i < 10000; ++i)
            std::swap(dataset[dis(gen)], dataset[dis(gen)]);
        std::cout << "打乱数据集" << std::endl;

        int train_size = dataset.size() * train_proportion;

        // 对数据进行归一化操作
        for (auto &[image, res] : dataset) {
            image.foreach([](float &val) {
                val /= 255;
            });
        }
        std::cout << "完成归一化" << std::endl;


        train_ = std::vector(dataset.begin(), dataset.begin() + train_size);
        test_ = std::vector(dataset.begin() + train_size, dataset.end());
        std::cout << "完成划分" << std::endl;

        for (int i = 0;i < train_count; ++i) {
            size_t acc_count{};
            for (const auto &[in, res] : train_) {
                train(in, res);

                FCLayer *back_layer = dynamic_cast<FCLayer *>(layouts_.back().get());
                int out_type = OneHot::out_to_type(back_layer->fc_out_);
                int res_type = OneHot::out_to_type(res);
                if (out_type == res_type)
                    acc_count ++;
            }
            std::cout << std::format("{}/{} , acc is {:.6f}", i, train_count, acc_count * 1.f / train_size) << '\r';
            std::cout.flush();
        }
        std::cout << std::endl;
        Console::show_cursor();
    }

};
