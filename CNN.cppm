/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2025/03/09 11:08
*******************************************************************************/


module;
#include <cmath>
#include <memory>
#include <vector>

#include "tools.h"
export module CNN;

import Matrix;
import Image;
import NeuralNetworkTool;


struct Layout {
    nl::Matrix<double> in, out;
    size_t kernel_size_{};
    size_t padding_{}, stride_{1}, channels_{1};

    virtual void forward(nl::Matrix<double> &in) = 0;
    virtual void background(nl::Matrix<double> &next_gradient) = 0;
    virtual ~Layout() = default;
};




class ConvLayer : public Layout {
    size_t kernel_count_{};
    std::vector<nl::Matrix<double>> kernels_;
    size_t image_size_{28};
    size_t kernel_size{3};
public:

    ConvLayer(size_t image_size, size_t kernel_count ,size_t kernel_size) : kernel_count_(kernel_count) {
        kernels_ = std::vector(kernel_count, nl::Matrix<double>(kernel_size,kernel_size, 1));
        // 卷积核全部初始化到 [-1 , 1]
        for (auto &kernel : kernels_) {
            kernel.random_init();
        }
    }

    // 前向传播
    void forward(nl::Matrix<double> &in) override {
        this->in = in;

        size_t out_size = (image_size_ + 2 * padding_ - kernel_size) / stride_ + 1;
        out = nl::Matrix<double>(out_size, out_size, kernel_count_);

        // 使用每个卷积核
        for (int k = 0; k < kernel_count_; ++k) {
            // 对于每个输出的元素
            for (int i = 0;i < out_size; ++ i) {
                for (int j = 0; j < out_size; ++ j) {

                    double sum{};
                    for (int x = 0; x < kernel_size; ++ x) {
                        for (int y = 0; y < kernel_size; ++ y) {
                            size_t input_x = i + x;
                            size_t input_y = j + y;
                            if (input_x < image_size_ && input_y < image_size_) {
                                for (int z = 0; z < channels_; ++ z) {
                                    sum += in[input_x, input_y, z] * kernels_[k][input_x, input_y, z];
                                }
                            }
                        }
                    }
                    out[i, j, k] = sum;
                }
            }
        }
    }

    // 反向传播
    void background(nl::Matrix<double> &next_gradient) override {

    }

};


class PoolLayout : Layout {
public:
    void forward(nl::Matrix<double> &in) override {
        this->in = in;

        // x == y
        size_t out_size = (in.x + 2 * padding_ - kernel_size_) / stride_ + 1;
        this->out = nl::Matrix<double>(out_size, out_size, in.z);

        for (int i = 0;i < out_size; ++i) {
            for (int j = 0;j < out_size; ++j) {
                for (int z = 0;z < in.z; ++z) {

                    auto max = std::numeric_limits<double>::min();
                    for (int x = 0;x < kernel_size_; ++x) {
                        for (int y = 0;y < kernel_size_; ++y) {
                            if (in[i + x, j + y, z] > max) {
                                max = in[i + x, j + y, z];
                            }
                        }
                    }
                    out[i, j, z] = max;
                }
            }
        }
    }

    void background(nl::Matrix<double> &next_gradient) override {

    }

};


class ReluLayout : Layout {
    std::shared_ptr<nl::Activation> activation_ = std::make_shared<nl::Relu>();

public:


    void forward(nl::Matrix<double> &in) override {
        this->in = in;
        this->out = nl::Matrix<double>(in.x, in.y, in.z);
        for (int x = 0; x < in.x; ++x) {
            for (int y = 0; y < in.y; ++y) {
                for (int z = 0; z < in.z; ++z) {
                    out[x, y, z] = activation_->action(in[x, y, z]);
                }

            }
        }
    }

    void background(nl::Matrix<double> &next_gradient) override {

    }


};

class FCLayout : Layout {

    std::vector<nl::Matrix<double>> weight_;
    std::vector<double> out_;
    std::shared_ptr<nl::Activation> activation_ = std::make_shared<nl::Sigmoid>();

    size_t out_size_{};

public:
    FCLayout(int x, int y, int z) {
        weight_ = std::vector(out_size_, nl::Matrix<double>(x, y, z));
        out_ = std::vector<double>(x * y * z);

        // 初始化权值矩阵
        for (int i = 0;i < weight_.size(); ++i) {
            weight_[i].random_init();
        }

    }

    void forward(nl::Matrix<double> &in) override {
        this->in = in;

        for (size_t pos{}; pos < out_size_; ++pos) {

            double sum{};
            for (int z = 0;z < weight_[pos].extent(0); ++z) {
                for (int x = 0;x < weight_[pos].extent(1);++x) {
                    for (int y = 0;y < weight_[pos].extent(2);++y) {
                        sum += weight_[pos][x,y,z] * in[x, y, z];
                    }
                }
            }
            out_[pos] = activation_->action(sum);
        }
    }

    void background(nl::Matrix<double> &next_gradient) override {

    }


};


export
NAMESPACE_BEGIN(nl)




class CNN {
public:



    // 训练
    void train(nl::Image image, nl::Matrix<double> label) {

    }

    // 预测
    nl::Matrix<double> forecast(nl::Image image) {

        return {};

    }


};




NAMESPACE_END(nl)


