/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/06 22:51
*******************************************************************************/

module;
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>
#include <vector>
export module modforge.deep_learning.bp;

import modforge.matrix;
import modforge.deep_learning.tools;

export
struct Layout {

    size_t size;
    Matrix<float> in, out, next_in;
    Matrix<float> weight, gradient;
    bool have_next{};

    std::shared_ptr<Activation> activation;


    explicit Layout(const size_t size, const std::shared_ptr<Activation> &action = std::make_shared<Sigmoid>())
            : size(size), in(1, size), out(1, size) , activation(action) {  }


    void forward(const Matrix<float> &pre_in) {
        in = pre_in;

        // 直接应用激活函数，没有应用线性变换
        for (int i = 0;i < in.extent(1); ++i)
            out[0, i] = activation->action(in[0, i]);

        if (have_next)
            next_in = out * weight;
    }

    void backward(const Matrix<float> &next_gradient) {
        

    }


};

export
struct BP {
    std::vector<std::shared_ptr<Layout>> layouts_;
    float speed = 0.001;

    std::vector<std::pair<std::initializer_list<float>, std::initializer_list<float>>> train_set_, test_set_;

public:

    BP() {

    }


    template <typename ... Args>
    BP(Args &&... args) requires (sizeof ...(args) >= 2 && (std::is_same_v<Args, int> && ...)) {
        for (auto size : {args...})
            add_layout(size);
    }

    void add_layout(size_t size) {
        add_layout(std::make_shared<Layout>(size));
    }
    void add_layout(Layout *layout) {
        add_layout(std::shared_ptr<Layout>(layout));
    }
    void add_layout(std::shared_ptr<Layout> layout) {

        if (!layouts_.empty()) {
            auto pre_layout = layouts_.back();
            pre_layout->have_next = true;
            pre_layout->weight = Matrix<float>(pre_layout->size, layout->size);
            pre_layout->weight.random_init();
        }
        layouts_.push_back(std::move(layout));

    }

    void train(const std::initializer_list<float> &in, const std::initializer_list<float> &out) {

        // 前向传播
        auto in_layout = layouts_.front();
        in_layout->forward(in);
        for (int i = 1;i < layouts_.size(); ++i)
            layouts_[i]->forward(layouts_[i-1]->next_in);


        // 反向传播
        auto out_layout = layouts_.back();


    }
    void train(std::vector<std::pair<std::initializer_list<float> , std::initializer_list<float>>>dataset,
        float train_proportion, size_t train_count, int seed) {

        int train_size = dataset.size() * train_proportion;


        train_set_ = std::vector(dataset.begin(), dataset.begin() + train_size);
        test_set_ = std::vector(dataset.begin() + train_size, dataset.end());

        // 随机打乱训练集
        int count = 1000;
        std::default_random_engine engine(seed);
        std::uniform_int_distribution<> dis(0, train_size - 1);
        for (int i = 0;i < count; ++i)
            std::swap(train_set_[dis(engine)], train_set_[dis(engine)]);

        // 开始训练
        for (int i = 0;i < train_count; ++i) {
            for (const auto &data : train_set_) {
                train(data.first, data.second);
            }
        }

    }

};