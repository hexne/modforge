/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <cassert>
#include <memory>
#include <numeric>
#include <vector>
export module modforge.deep_learning.cnn;


import modforge.tensor;

import modforge.deep_learning.bp;
import modforge.deep_learning.tools;


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
    ConvLayer(size_t n , size_t size, FeatureExtent in_extent, size_t stride = 1, size_t padding = 0) {
        this->in_extent = in_extent;
        stride_ = stride;
        padding_ = padding;
        kernels_ = Kernels(n, in_extent.cannel, size, size);
        out_extent = get_out_extent(this->in_extent, kernels_, stride, padding);
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

    void backward(const FeatureMap &) override {

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
    ActionLayer() = default;

    explicit ActionLayer(std::shared_ptr<Activation> action) : action_(std::move(action)) {  }

    void forward(const FeatureMap &in) override {
        this->in = in;
        this->out = this->in.copy();
        out.foreach([this](float &val) {
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
    Kernels weight_;
    Vector<float> fc_in_, fc_out_;
    std::shared_ptr<Activation> action_ = std::make_shared<Sigmoid>();
public:
    FCLayer(const FeatureExtent &in_extent) {
        this->out_extent = this->in_extent = in_extent;
        out_extent.h = out_extent.w = 1;

        weight_ = Kernels(out_extent.cannel, in_extent.cannel, out_extent.h, out_extent.w);
        fc_in_ = Vector<float>(out_extent.cannel);
        fc_out_ = Vector<float>(out_extent.cannel);

        random_tensor(weight_, -1, 1);

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

    }

    // 全链接层输出
    void backward(const Vector<float> &next_gradient) {

        Vector<float> vector_gradient = next_gradient;
        for (int i = 0;i < next_gradient.size(); ++i) {
            vector_gradient[i] *= action_->deaction(fc_in_[i]);
        }

        // @TODO 学习率暂定
        // 修改权值
        int speed = 0.001;
        for (int c_out = 0; c_out < out_extent.cannel; ++c_out) {
            for (int c_in = 0; c_in < in_extent.cannel; ++c_in) {
                for (int h = 0; h < in_extent.h; ++h) {
                    for (int w = 0; w < in_extent.w; ++w) {
                        weight_[c_out, c_in, h, w] -= speed * vector_gradient[c_out] * in[c_in, h, w];
                    }
                }
            }
        }

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
    void backward(const FeatureMap &next_gradient) override {

        // 什么也不干

    }

};

// 输入层
class InputLayer : public CNNLayer {
public:
    InputLayer(const FeatureExtent &extent) {
        // 输入层不对extent进行变换
        in_extent = extent;
        out_extent = in_extent;
    }
    void forward(const FeatureMap &in) override {
        this->in = in;
        this->in.foreach([](float &val){
            val /= 255;         // 归一化操作可进行扩充, 暂时先用这个
        });

    }

    void backward(const FeatureMap &next_gradient) override {

    }
};


export
class CNN {
    std::vector<std::shared_ptr<CNNLayer>> layouts_;

public:
    // 构造的时候就创建第一层，用于确定输入和输出的尺寸
    CNN(int w, int h, int cannel) {
        FeatureExtent in_extent { .w = w, .h = h, .cannel = cannel };
        layouts_.emplace_back(std::make_shared<InputLayer>(in_extent));
    }

    void add_convlayer(size_t n, size_t size, const FeatureExtent &in_extent, size_t stride = 1, size_t padding = 0) {
        auto layer = std::make_shared<ConvLayer>(n, size, layouts_.back()->out_extent, stride, padding);
        layouts_.emplace_back(layer);
    }


    void forward(const FeatureMap &in) {

    }

};
