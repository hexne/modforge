/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2025/03/09 11:08
*******************************************************************************/


module;
#include <mdspan>
#include <random>
#include <array>
#include "tools.h"
export module CNN;


// 卷积层
class ConvLayer {
    static constexpr size_t kernel_row{3}, kernel_col{3}, kernel_count{5};
    static constexpr size_t image_weight{255}, image_height{255};

    std::array<std::array<double,kernel_row * kernel_col>, kernel_count> kernels_data_{};
    std::array<std::mdspan<double, std::dextents<size_t, 2>>, kernel_count> kernels_ {};
    std::array<std::array<double, image_weight * image_height>, kernel_count> trait_data_{};
    std::array<std::mdspan<double, std::dextents<size_t, 2>>, kernel_count> trait_ {};
    std::mdspan<unsigned char, std::dextents<size_t, 2>> image_;

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

    }
public:
    explicit ConvLayer(unsigned char * image) : image_(std::mdspan(image, image_height, image_weight)) {
        init_kernels();
    }

    void convolution() {

        for (int count = 0; count < kernel_count; count++) {
            for (int i = 0;i < image_height - kernel_row; ++i) {
                for (int j = 0;j < image_weight - kernel_col; ++j) {
                    for (int x = 0; x < kernel_row; ++x) {
                        for (int y = 0; y < kernel_col; ++y) {
                            trait_data_[count][i * image_weight + j] = kernels_[count][x, y] * image_[i + x, j + y];
                        }

                    }
                }
            }
        }


    }


};

class NonLinearLayer {  };
class PoolLayer {  };
class FullyConnLayer {  };



export
NAMESPACE_BEGIN(nl)





class CNN {

};

NAMESPACE_END(nl)


