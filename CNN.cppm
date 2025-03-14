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

struct Layout {
    virtual ~Layout() = default;
    virtual void forward() = 0;
    virtual void background() = 0;
};

// 卷积层
class ConvLayer : public Layout {

    static constexpr size_t colors{3};
    static constexpr size_t kernel_row{3}, kernel_col{3}, kernel_count{5};
    static constexpr size_t image_weight{512}, image_height{255};
    static constexpr size_t stride{1};


    static constexpr std::array<std::size_t, 2> view_strides = { image_weight * colors, colors};
    static constexpr std::layout_stride::mapping<std::dextents<size_t, 2>> image_layout
                = std::layout_stride::mapping(std::dextents<size_t, 2> { image_height, image_weight }, view_strides);


    // 内核数据 和 数据的二维视图
    std::array<std::array<double,kernel_row * kernel_col>, kernel_count> kernels_data_{};
    std::array<std::mdspan<double, std::dextents<size_t, 2>>, kernel_count> kernels_ {};

    // 特征数据, @TODO, 暂时只有一张特征图
    std::array<unsigned char, image_weight * image_height * colors> trait_data_{};

    // 图像和特征 的二维视图
    std::mdspan<unsigned char, std::dextents<size_t, 2>> image_, trait_;

    // 图像和特征各个通道 的二维视图
    std::array<std::mdspan<unsigned char, std::dextents<size_t, 2>, std::layout_stride>, colors> rgb_, trait_rgb_;



    void init_kernels() {
        // kernels_data_ = {
        //     {
        //         0.f, 1.f, 0.f,
        //         0.f, 1.f, 0.f,
        //         0.f, 1.f, 0.f,
        //     },
        // };

        static std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution dis(-1.0, 1.0);

        int count{};
        while (count++ < kernel_count) {
            int pos{};
            while (pos < kernel_row * kernel_col) {
                kernels_data_[count][pos++] = dis(gen);
            }
        }


        for (int i = 0; i < kernel_count; i++)
            kernels_[i] = std::mdspan(kernels_data_[i].data(), kernel_row, kernel_col);

        kernels_data_[0] = {
            1, 0, 1,
            1, 0, 1,
            1, 0, 1,
        };

    }
    static double normalize(const unsigned char num) {
        return static_cast<double>(num) / 255;
    }
    static unsigned char denormalize(const double num) {
        if (num > 1)
            return 255;
        return static_cast<unsigned char>(num * 255);
    }
public:

    explicit ConvLayer(unsigned char * image) : image_(std::mdspan(image, image_height, image_weight)),
            trait_(std::mdspan(trait_data_.data(), image_height, image_weight)) {

        init_kernels();

        for (int i = 0;i < colors; ++i)
            rgb_[i] = std::mdspan(image + i, image_layout);

        for (int i = 0;i < colors; ++i)
            trait_rgb_[i] = std::mdspan(trait_data_.data() + i, image_layout);

    }

    void convolution() {

        for (int cur_kernel{}; cur_kernel < 1; ++cur_kernel) {
            for (int cur_color{}; cur_color < colors; ++cur_color) {
                // 遍历图像
                for (int i = 0;i < image_height - (kernel_row+1)/2; ++i) {
                    for (int j = 0;j < image_weight - (kernel_col+1)/2; ++j) {

                        double sum{};
                        for (int x = 0; x < kernel_row; ++x) {
                            for (int y = 0; y < kernel_col; ++y) {
                                // sum += normalize(rgb_[cur_color][i + x, j + y] * kernels_[cur_kernel][x, y]);
                                sum += rgb_[cur_color][i + x, j + y] * kernels_[cur_kernel][x, y];
                                // ... 卷积出一个结果
                            }
                            // std::cout << std::endl;
                        }
                        // std::cout << std::endl;
                        // trait_rgb_[cur_color][i, j] = denormalize(sum / (kernel_row * kernel_col));
                        trait_rgb_[cur_color][i, j] = sum;
                    }
                }
            }
            // @TODO 暂时仅有一个卷积核
        }
        cv::Mat image(image_height, image_weight, CV_8UC3, trait_data_.data());
        cv::imshow("image", image);
        cv::waitKey();
    }
    void forward() override {

    }
    void background() override {

    }


};

// 非线性激励层
class ReLULayer : public Layout {

public:
    void forward() override {

    }
    void background() override {

    }



};

// 池化层
class PoolLayer : public Layout {
public:
    void forward() override {

    }
    void background() override {

    }
};

// 全链接层
class FullyConnLayer : public Layout {
public:
    void forward() override {

    }
    void background() override {

    }
};



export
NAMESPACE_BEGIN(nl)





class CNN {
    std::vector<Layout *> layouts_;


public:

    void add_convlayout() {  }
    void add_poollayout() {  }
    void add_relulayout() {  }

    void start(nl::Image image) {  }



};

NAMESPACE_END(nl)


