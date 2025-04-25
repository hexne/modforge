/*******************************************************************************
 * @Author : hexne
 * @Data   : 2025/03/09 11:08
*******************************************************************************/


module;
#include <cmath>
#include <memory>
#include <vector>
#include <opencv2/core.hpp>

#include "tools.h"
export module CNN;

import Matrix;
import Image;
import NeuralNetworkTool;


struct Layout {
    friend class CNN;

    nl::Matrix<double> in, out, gradient;
    size_t kernel_size_{};
    size_t padding_{}, stride_{1}, channels_{1};
    double speed = 0.001;

    virtual void forward(nl::Matrix<double> &in) = 0;
    virtual void background(nl::Matrix<double> &next_gradient) = 0;
    virtual ~Layout() = default;
};

export
NAMESPACE_BEGIN(nl)

class ConvLayer : public Layout {
    size_t kernel_count_{};
    std::vector<nl::Matrix<double>> kernels_, kernel_gradient_;
    size_t image_size_{28};
    size_t kernel_size{3};
public:

    ConvLayer(size_t kernel_count ,size_t kernel_size) : kernel_count_(kernel_count) {
        kernels_ = std::vector(kernel_count, nl::Matrix<double>(kernel_size,kernel_size, 1));
        kernel_gradient_ = std::vector(kernel_count, nl::Matrix<double>(kernel_size,kernel_size, 1));
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
                            size_t input_x = i * stride_ + x;
                            size_t input_y = j * stride_ + y;
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

        for (int i{}; i < kernel_gradient_.size(); ++i) {
            for (int x{}; x < kernel_gradient_[i].x; ++x) {
                for (int y{}; y < kernel_gradient_[i].y; ++y) {
                    for (int z{}; z < kernel_gradient_[i].z; ++z) {
                        kernel_gradient_[i][x, y, z] = 0;
                    }
                }
            }
        }



        // 卷积层梯度有两种
        // 1、卷积核的梯度，用于修改卷积核权重
        // 2、保存的梯度，用于上一层反向传播使用
        for (int x{}; x < in.x; ++x) {
            for (int y{}; y < in.y; ++y) {
                auto [min_x, min_y, min_z, max_x, max_y, max_z] = nl::GetRange(x, y, kernel_size, stride_, channels_);

                for (int z{}; z < in.z; ++z) {

                    double sum{};
                    // 卷积层在前向传播时是三维卷积
                    for (int i = min_x; i <= max_x; ++i) {
                        for (int j = min_y; j <= max_y; ++j) {
                            for (int k = min_z; k <= max_z; ++k) {
                                sum += kernels_[z][ x - min_x, y - min_y, z] * gradient[i, j, k];
                                kernel_gradient_[z][x - min_x, y - min_y, z] += in[x, y, z] * gradient[i, j, k];
                            }
                        }
                    }

                    gradient[x, y, z] = sum;
                }
            }
        }


        // 修改权重
        for (int i = 0; i < kernels_.size(); ++i) {

            for (int x{}; x < kernels_[i].x; ++x) {
                for (int y{}; y < kernels_[i].y; ++y) {
                    for (int z{}; z < kernels_[i].z; ++z) {
                        kernels_[i][x, y, z] -= speed * kernel_gradient_[i][x, y, z];
                    }
                }
            }

        }



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
                                max = in[i * stride_ + x, j * stride_ + y, z];
                            }
                        }
                    }
                    out[i, j, z] = max;
                }
            }
        }
    }

    void background(nl::Matrix<double> &next_gradient) override {

        // 最大池化对应的反向传播
        for (int x = 0; x < in.x; x ++) {
            for (int  y = 0; y < in.y; y ++) {
                for (int z = 0; z < in.z; z ++) {

                    auto [min_x, min_y, min_z, max_x, max_y, max_z] = nl::GetRange(x, y, kernel_size_, stride_, channels_);

                    double sum{};
                    for (int i = min_x; i < max_x; i ++) {
                        for (int j = min_y; j < max_y; j ++) {

                            int flag = 0;
                            if (in[x, y, z] == out[i, j ,z])
                                flag = 1;
                            sum += flag * next_gradient[i, j, z];
                        }
                    }
                    gradient[x, y, z] = sum;
                }
            }
        }

        // 池化层不需要更新权重

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
        for (int x = 0; x < next_gradient.x; ++x) {
            for (int y = 0; y < next_gradient.y; ++y) {
                for (int z = 0; z < next_gradient.z; ++z) {
                    gradient[x, y, z] = activation_->action(in[x, y, z]) * next_gradient[x, y, z];
                }
            }
        }
        // 非线性激励层无需调整权值，因此只需要计算梯度供上一层反向传播使用即可
    }


};

class FCLayout : Layout {

    std::vector<nl::Matrix<double>> weight_;

    // 此处的in_, out_ 为和权值矩阵点乘之后展平的输出输出
    std::vector<double> in_, out_;
    std::shared_ptr<nl::Activation> activation_ = std::make_shared<nl::Sigmoid>();
    std::shared_ptr<nl::LossFunction> loss_function_ = std::make_shared<nl::MeanSquaredError>();

    size_t out_size_{};
    std::vector<double> gradient_;

public:
    FCLayout(int x, int y, int z) {
        weight_ = std::vector(out_size_, nl::Matrix<double>(x, y, z));
        in_ = out_ = std::vector<double>(x * y * z);

        // 初始化权值矩阵
        for (auto &weight : weight_) {
            weight.random_init();
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
            in_[pos] = sum;
            out_[pos] = activation_->action(in_[pos]);
        }
    }

    // 参数为结果的真实值
    void background(nl::Matrix<double> &next_gradient) override {

        // gradient_ 中保存的是
        // gradient 中保存的是上一层进行反向传播的时候需要使用的梯度
        for (int i = 0;i < weight_.size(); ++i) {
            gradient_[i] = loss_function_->action(out_[i], next_gradient[i, 0, 0]) * activation_->deaction(in_[i]);
        }

        for (int x = 0;x < gradient.x; ++x) {
            for (int y = 0;y < gradient.y; ++y) {
                for (int z = 0;z < gradient.z; ++z) {
                    double sum{};
                    for (int i = 0;i < weight_.size(); ++i) {
                        sum += weight_[i][x, y, z] * gradient_[i];
                    }
                    gradient[x, y, z] = sum;
                }
            }
        }


        // 更新权重
        for (int i = 0;i < weight_.size(); ++i) {

            for (int x = 0; x < in.x; ++x) {
                for (int y = 0; y < in.y; ++y) {
                    for (int z = 0; z < in.z; ++z) {
                        // 权重更新直接使用 SGD(随机梯度下降)
                        weight_[i][x, y, z] -= speed * gradient_[i] * in[x, y, z];
                    }
                }
            }

        }



    }


};






class CNN {
    std::vector<std::shared_ptr<Layout>> layouts_;
    size_t in_size_, out_size_;
public:

    // 参数为图像大小 和 输出的结果大小
    CNN(const size_t in_size, const size_t label_size) : in_size_(in_size), out_size_(label_size) {  }

    void add_layout(std::shared_ptr<Layout> &layout) {
        layouts_.push_back(layout);
    }

    // 训练
    void train(nl::Image &image, nl::Matrix<double> &label) {
        auto matrix = image.to_matrix();

        // @TODO 图像应该被预处理
        // 预处理后的数据传入第一个layout的forward中

        auto front_layout = layouts_.front();

        nl::Matrix<double> todo_matrix;
        front_layout->forward(todo_matrix);
        for (int i = 1; i < layouts_.size(); ++i) {
            layouts_[i]->forward(layouts_[i-1]->out);
        }


        // 最后一层直接传入真实值
        // @TODO 真实值应该被处理为输出类型的矩阵
        auto back_layout = layouts_.back();
        back_layout->background(label);
        for (int i = layouts_.size() - 2; i >= 0; --i) {
            layouts_[i]->background(layouts_[i+1]->gradient);
        }


    }

    // 预测
    nl::Matrix<double> forecast(nl::Image &image) {
        auto front_layout = layouts_.front();

        nl::Matrix<double> todo_matrix;
        front_layout->forward(todo_matrix);
        for (int i = 1; i < layouts_.size(); ++i) {
            layouts_[i]->forward(layouts_[i-1]->out);
        }
        return layouts_.back()->out;
    }


};




NAMESPACE_END(nl)


