/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2025/03/09 11:08
*******************************************************************************/


module;
#include <mdspan>
#include <random>
#include <array>
#include <opencv2/opencv.hpp>
#include "tools.h"
export module CNN;

import Image;


template <typename T>
class Tensor {
    std::shared_ptr<T []> data_;
    std::mdspan<T, std::dextents<size_t,3>> view_;
public:
    int x{},y{},z{};
    Tensor(int x, int y, int z = 1) : x(x), y(y), z(z) {
        auto p = new T[x * y * z];
        new (this) Tensor(p, x, y, z);
    }
    Tensor(T * data, int x, int y, int z = 1) : x(x), y(y), z(z) {
        data_ = std::shared_ptr<T[]>(data);
        view_ = std::mdspan(data_.get(), z, x, y);
    }
    T &operator[] (int x, int y, int z) {
        return view_[z, x, y];
    }
    size_t extent(int num) {
        return view_.extent(num);
    }

    Tensor operator - (const Tensor &other) const {
        int z = view_.extent(0), x = view_.extent(1), y = view_.extent(2);
        Tensor ret(z, x, y);
        for (int i = 0; i < z * x * y; ++i) {
            ret[i] = data_[i] - other[i];
        }
        return ret;
    }
    Tensor operator + (const Tensor &other) const {
        int z = view_.extent(0), x = view_.extent(1), y = view_.extent(2);
        Tensor<T> ret(z, x, y);
        for (int i = 0; i < z * x * y; ++i) {
            ret[i] = data_[i] + other[i];
        }
        return ret;
    }
};

struct Layout {
    virtual Tensor<float> forward(const Tensor<float> &) = 0;
    virtual void background() = 0;
    virtual ~Layout() = default;
};


template <typename T>
struct ImageLabelPair {
    Tensor<T> image;
    Tensor<T> label;
};


class ConvLayer : public Layout {
    size_t kernel_count_{};
    std::vector<Tensor<float>> kernels_;
    size_t image_size_{28}, padding_{}, stride_{1}, channels_{1};





public:
    size_t kernel_size{3};
    ConvLayer(int kernel_count) : kernel_count_(kernel_count) {
        kernels_ = std::vector<Tensor<float>> (kernel_count, Tensor<float>(kernel_size,kernel_size, 1));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution dis(0.f,1.f);

        // 初始化kernel
        for (auto &kernel : kernels_) {
            for (int i = 0;i < kernel.extent(0); ++i) {
                for (int j = 0;j < kernel.extent(1); ++j) {
                    for (int k = 0;k < kernel.extent(2); ++k) {
                        kernel[i, j, k] = dis(gen);
                    }
                }

            }
        }



    }
    Tensor<float> forward(Tensor<float> &data) {

        size_t out_size = (image_size_ + 2 * padding_ - kernel_size) / stride_ + 1;
        Tensor<float> out(out_size, out_size, kernel_count_);

        // 使用每个卷积核
        for (int k = 0; k < kernel_count_; ++k) {
            // 对于每个输出的元素
            for (int i = 0;i < out_size; ++ i) {
                for (int j = 0; j < out_size; ++ j) {

                    float sum{};
                    for (int x = 0; x < kernel_size; ++ x) {
                        for (int y = 0; y < kernel_size; ++ y) {
                            size_t input_x = i + x;
                            size_t input_y = j + y;
                            if (input_x < image_size_ && input_y < image_size_) {
                                for (int z = 0; z < channels_; ++ z) {
                                    sum += data[input_x, input_y, z] * kernels_[k][input_x, input_y, z];
                                }
                            }
                        }
                    }
                    out[i, j, k] = sum;
                }
            }
        }
        return out;
    }
    void background() {

    }

};


template <typename T>
class PoolLayout {
    size_t kernel_size_{};
    size_t padding_{}, stride_{1}, channels_{1};
public:
    PoolLayout() {

    }

    Tensor<float> forward(Tensor<float> &data) {

        // x == y
        size_t out_size = (data.x + 2 * padding_ - kernel_size_) / stride_ + 1;
        Tensor<float> out(out_size,out_size, data.z);

        for (int i = 0;i < out_size; ++i) {
            for (int j = 0;j < out_size; ++j) {
                for (int z = 0;z < data.z; ++z) {

                    float max = INT_MIN * 1.f;
                    for (int x = 0;x < kernel_size_; ++x) {
                        for (int y = 0;y < kernel_size_; ++y) {
                            if (data[i + x, j + y, z] < max) {
                                max = data[i + x, j + y, z];
                            }
                        }
                    }
                    out[i, j, z] = max;
                }
            }
        }
        return out;
    }

    void background() {

    }

};


template <typename T>
class ReluLayout {

public:
    ReluLayout() = default;

    float activator(float x) const {
        if (x > 0)
            return x;
        return 0;
    }
    Tensor<float> forward(Tensor<float> &data) {
        Tensor<float> out(data.x, data.y, data.z);
        for (int x = 0; x < data.x; ++x) {
            for (int y = 0; y < data.y; ++y) {
                for (int z = 0; z < data.z; ++z) {
                    out[x, y, z] = activator(data[x, y, z]);
                }

            }
        }
        return out;
    }
    void background() {

    }

};

template <typename T>
class FCLayout {

    std::vector<Tensor<float>> weight_;
    std::vector<float> out_;

    size_t out_size_{};
    float activator(float x) const {
        return 1.f / (1 + exp(-x));
    }
    float deactivator(float x) const {
        float sigmod = 1.f / (1 + exp(-x));
        return sigmod * (1 - sigmod);
    }
public:
    FCLayout(size_t x, size_t y, size_t z, size_t out_size) : out_size_(out_size) {
        weight_ = std::vector(out_size_, Tensor<float>(x, y, z));
        out_ = std::vector<float>(x * y * z);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution dis(0.f,1.f);

        for (int count = 0;count < weight_.size(); ++count) {
            for (int z = 0;z < weight_[count].extent(0); ++z) {
                for (int x = 0;x < weight_[count].extent(1);++x) {
                    for (int y = 0;y < weight_[count].extent(2);++y) {
                        weight_[count][x,y,z] = dis(gen);
                    }
                }
            }
        }



    }

    Tensor<T> forward(Tensor<float> &data) {

        for (size_t pos{}; pos < out_size_; ++pos) {

            float sum{};
            for (int z = 0;z < weight_[pos].extent(0); ++z) {
                for (int x = 0;x < weight_[pos].extent(1);++x) {
                    for (int y = 0;y < weight_[pos].extent(2);++y) {
                        sum += weight_[pos][x,y,z] * data[x, y, z];
                    }
                }
            }
            out_[pos] = activator(sum);
        }
        return {};
    }

    void background() {

    }


};


export
NAMESPACE_BEGIN(nl)




class CNN {
    int label_count{};

public:

    CNN(const nl::Image image, Tensor<float> label) {

    }

};



NAMESPACE_END(nl)


