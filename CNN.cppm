/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2025/03/09 11:08
*******************************************************************************/


module;
#include <mdspan>
#include <random>
#include "tools.h"
export module CNN;


// 卷积层
class ConvLayer {
    static constexpr size_t kernel_row{3}, kernel_col{3}, kernel_count{5};

    std::array<std::array<double,kernel_row * kernel_col>, kernel_count> kernels_data_{};
    std::array<std::mdspan<double, std::extents<size_t, kernel_row,kernel_col>>, kernel_count> kernels_{};

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
            kernels_[i] = std::mdspan<double, std::extents<size_t, kernel_row, kernel_col>> (kernels_data_[i].data(), kernel_row, kernel_col);

    }
public:
    ConvLayer() {
        init_kernels();

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


