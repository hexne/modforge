/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
export module modforge.deep_learning.cnn;

import modforge.tensor;
import modforge.console;
import modforge.progress;
import modforge.deep_learning.tools;


using PoolWindow = Tensor<float, 2>;
using FeatureMap = Tensor<float, 3>;
using Kernels    = Tensor<float, 4>;
using Label      = Vector<float>;
struct FeatureExtent {
    int w{};
    int h{};
    int cannel{};
};
struct Data {
    FeatureMap feature_map;
    Label label;
};

int32_t swap_endian(int32_t val) {
    return ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) |
           ((val >> 8) & 0xff00) | ((val << 24) & 0xff000000);
}

std::vector<Data> load_dataset_impl(const std::string &path_images, const std::string &path_labels) {
    std::vector<Data> ret;

    std::ifstream images_in(path_images, std::ios::binary);
    if (!images_in) throw std::runtime_error("Failed to open images file: " + path_images);

    int32_t magic, num_images, rows, cols;
    images_in.read(reinterpret_cast<char*>(&magic), 4);
    images_in.read(reinterpret_cast<char*>(&num_images), 4);
    images_in.read(reinterpret_cast<char*>(&rows), 4);
    images_in.read(reinterpret_cast<char*>(&cols), 4);

    magic = swap_endian(magic);
    num_images = swap_endian(num_images);
    rows = swap_endian(rows);
    cols = swap_endian(cols);

    std::ifstream labels_in(path_labels, std::ios::binary);
    if (!labels_in) throw std::runtime_error("Failed to open labels file: " + path_labels);

    int32_t label_magic, label_count;
    labels_in.read(reinterpret_cast<char*>(&label_magic), 4);
    labels_in.read(reinterpret_cast<char*>(&label_count), 4);
    label_magic = swap_endian(label_magic);
    label_count = swap_endian(label_count);

    if (magic != 2051 || label_magic != 2049 || num_images != label_count)
        throw std::runtime_error("Invalid MNIST file format");

    std::vector<uint8_t> image_data(num_images * rows * cols);
    std::vector<uint8_t> label_data(num_images);

    images_in.read(reinterpret_cast<char*>(image_data.data()), image_data.size());
    labels_in.read(reinterpret_cast<char*>(label_data.data()), label_data.size());

    constexpr int num_classes = 10;
    ret.reserve(num_images);

    Progressbar pb("加载数据...", num_images);
    for (int i = 0; i < num_images; ++i) {
        pb += 1;
        pb.print();

        FeatureMap image(1, rows, cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                image[0, r, c] = static_cast<float>(image_data[i * rows * cols + r * cols + c]);
            }
        }

        Label label = OneHot::type_to_onehot<num_classes>(label_data[i]);

        ret.emplace_back(Data{image, label});
    }
    // std::endl(std::cout);

    return ret;
}

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

// 更新权重
void update_weight() {

}
// 更新梯度
void update_gradient() {

}

struct Layer {
    FeatureMap in, out, gradient;
    FeatureExtent in_extent, out_extent;

    virtual void forward(const FeatureMap &) = 0;
    // virtual void backward() = 0;
    virtual ~Layer() = default;
};

class ConvLayer : public Layer {
    size_t stride_, padding_;
    Kernels kernels_;
public:
    ConvLayer(size_t n , size_t size, FeatureExtent in_extent, size_t stride = 1, size_t padding = 0)
           : stride_(stride), padding_(padding) {

        this->in_extent = in_extent;
        out_extent = get_out_extent(in_extent, kernels_, stride, padding);

        kernels_ = Kernels(n, in_extent.cannel, size, size);
        int N = in_extent.w * in_extent.h * in_extent.cannel;

        kernels_.foreach([=](float &val) {
            val = 1.0f / N * rand() / 2147483647.0; //随机的值是有讲究的，这个是CNN常用的卷积核随机初值设置
        });

        // @TODO 梯度
        // gradient = Kernels(n, in_extent.cannel, size, size);
    }

    void forward(const FeatureMap &in) override {
        this->in = in;

        int kernel_z = kernels_.extent(1);
        int kernel_h = kernels_.extent(2);
        int kernel_w = kernels_.extent(3);

        this->out = FeatureMap(out_extent.cannel, out_extent.h, out_extent.w);

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

                                sum += this->in[kz, ih, iw] * kernels_[z, kz, kh, kw];
                            }
                        }
                    }

                    this->out[z, x, y] = sum;
                }
            }
        }
    }

};
class PoolLayer : public Layer {
    PoolWindow pool_window_;
    size_t stride_;

public:
    PoolLayer(size_t window_size, const FeatureExtent &in_extent, size_t stride = 1, size_t padding = 0) {
        this->in_extent = in_extent;
        stride_ = stride;
        pool_window_ = PoolWindow(window_size, window_size);
        out_extent = get_out_extent(this->in_extent, pool_window_, stride, padding);
    }

    void forward(const FeatureMap &in) override {
        this->in = in;

        const int pool_h = pool_window_.extent(0);
        const int pool_w = pool_window_.extent(1);

        this->out = FeatureMap(out_extent.cannel, out_extent.h, out_extent.w);

        for (int cur_cannel = 0; cur_cannel < out_extent.cannel; ++cur_cannel) {
            for (int oh = 0; oh < out_extent.h; ++oh) {
                for (int ow = 0; ow < out_extent.w; ++ow) {

                    float cur_max = std::numeric_limits<float>::lowest();

                    const int start_x = oh * stride_;
                    const int start_y = ow * stride_;

                    for (int ph = 0; ph < pool_h; ++ph) {
                        for (int pw = 0; pw < pool_w; ++pw) {
                            const int in_x = start_x + ph;
                            const int in_y = start_y + pw;

                            if (cur_max < in[cur_cannel, in_x, in_y])
                                cur_max = in[cur_cannel, in_x, in_y];
                        }
                    }

                    this->out[cur_cannel, oh, ow] = cur_max;
                }
            }
        }
    }
};
class ActionLayer : public Layer {
    std::shared_ptr<Activation> action_;
public:
    ActionLayer(const FeatureExtent& in_extent ,std::shared_ptr<Activation> action = std::make_shared<Sigmoid>())
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
};

export class CNN;
class FCLayer : public Layer {
    friend class CNN;
    Kernels weight_;
    Vector<float> fc_in_, fc_out_;
    std::shared_ptr<Activation> action_ = std::make_shared<Sigmoid>();

public:
    FCLayer(int type_count, const FeatureExtent &in_extent) {
        this->in_extent = in_extent;

        out_extent = FeatureExtent { .w = 1, .h = 1, .cannel = type_count };

        weight_ = Kernels(out_extent.cannel, in_extent.cannel, in_extent.h, in_extent.w);
        fc_in_ = Vector<float>(out_extent.cannel);
        fc_out_ = Vector<float>(out_extent.cannel);
        int N = in_extent.h * in_extent.w * in_extent.cannel;

        weight_.foreach([=](float &val) {
            val = 2.19722f / N * rand() / float( RAND_MAX );
        });

    }

    void forward(const FeatureMap &in) override {
        this->in = in;

        for (int cur_kernel = 0; cur_kernel < out_extent.cannel; ++cur_kernel) {
            float sum{};
            for (int z = 0; z < in_extent.cannel; ++z) {
                for (int x = 0; x < in_extent.h; ++x) {
                    for (int y = 0; y < in_extent.w; ++y) {
                        sum += weight_[cur_kernel, z, x, y] * in[z, x, y];
                    }
                }
            }
            fc_in_[cur_kernel] = sum;
            fc_out_[cur_kernel] = action_->action(sum);
        }
    }
};


class CNN {
    FeatureExtent in_extent;
    std::vector<std::shared_ptr<Layer>> layers_;
    std::vector<Data> train_, test_;
    std::shared_ptr<LossFunction> loss_ = std::make_shared<MeanSquaredError>();


    [[nodiscard]]
    FeatureExtent get_cur_in_extent() const {
        FeatureExtent ret;
        if (layers_.empty())
            ret = in_extent;
        else
            ret = layers_.back()->out_extent;
        return ret;
    }


    void process_data() {

    }

public:
    std::function<std::vector<Data>(const std::string &, const std::string &)> load_dataset = load_dataset_impl;

    CNN(int w, int h, int cannel) {
        in_extent = FeatureExtent{ .w = w, .h = h, .cannel = cannel };
    }

    void add_conv_layer(size_t n, size_t size, size_t stride = 1, size_t padding = 0) {
        auto layer = std::make_shared<ConvLayer>(n, size, get_cur_in_extent(), stride, padding);
        layers_.emplace_back(layer);
    }

    void add_action_layer(std::shared_ptr<Activation> action = std::make_shared<Relu>()) {
        auto layer = std::make_shared<ActionLayer>(get_cur_in_extent(), action);
        layers_.emplace_back(layer);
    }

    void add_pool_layer(size_t window_size, size_t stride = 2) {
        auto layer = std::make_shared<PoolLayer>(window_size, get_cur_in_extent(), stride);
        layers_.emplace_back(layer);
    }

    void add_fc_layer(size_t type_count) {
        auto layer = std::make_shared<FCLayer>(type_count, get_cur_in_extent());
        layers_.emplace_back(layer);
    }

    void forward(const FeatureMap &in) {
        for (auto &layer : layers_) {
            layer->forward(in);
        }
    }
    // 在 CNN 类中
    void backward(const Vector<float> &res) {


    }


    void train(const FeatureMap &in, const Vector<float> &out) {
        forward(in);
        backward(out);
    }
    void train(const std::string &feature_path, const std::string &label_path,
        float train_proportion, size_t train_count, int seed) {

        if (!load_dataset)
            throw std::runtime_error("load_dataset is not define");

        auto dataset = load_dataset(feature_path, label_path);

        for (const auto &[feature_map, label] : dataset)
            train(feature_map, label);

    }
};




/****************************** 使用示例 ******************************\
import modforge.deep_learning.cnn;


int main(int argc, char *argv[]) {

    auto dataset = load_dataset("./dataset/train-images.idx3-ubyte", "./dataset/train-labels.idx1-ubyte");


    return 0;
}
*/