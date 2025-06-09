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

import modforge.tensor;
import modforge.deep_learning.tools;
import modforge.average_queue;
import modforge.console;

int cur_train_count;
int train_count;
std::shared_ptr<Optimizer> optimizer;

export
struct Layout {
    size_t size;
    Vector<float> in, out, next_in{}, gradient{};
    Tensor<float, 2> weight{};
    bool have_next{};

    std::shared_ptr<Activation> action;


    Layout(const size_t size, const std::shared_ptr<Activation> &action = std::make_shared<Relu>())
        : size(size), in(size), out(size) , action(action) {  }

    void forward(const Vector<float> &pre_in) {
        in = pre_in;

        for (int i = 0;i < in.size(); ++i)
            out[i] = action->action(in[i]);

        if (have_next)
            next_in = out * weight;
    }
    void backward(const Vector<float> &next_gradient) {
        gradient = next_gradient;

        // 如果不是最后一层
        if (have_next) {

            double speed = 0.001;
            if (!optimizer)
                speed = optimizer->get_speed(cur_train_count, train_count);

            for (int i = 0;i < weight.extent(0); ++i)
                for (int j = 0;j < weight.extent(1); ++j)
                    weight[i, j] -= speed * gradient[j] * out[i];


            auto t = Tensor<float, 2>::from_view(weight);
            t.transpose();
            gradient = gradient * t;
        }

        for (int i = 0;i < out.size(); ++i)
            gradient[i] *= action->deaction(in[i]);

    }

};

template<typename T>
double mean_relative_error(Vector<T>& output, Vector<T>& target) {
    double error = 0.0;
    int count = 0;
    for(int i = 0; i < output.size(); ++i) {
        if(target[i] != 0) {  // 避免除以0
            error += std::abs(output[i] - target[i]) / std::abs(target[i]);
            count++;
        }
    }
    return count > 0 ? error / count : 0.0;
}

export
class BP {
    std::vector<std::shared_ptr<Layout>> layouts_;
    std::shared_ptr<LossFunction> loss_ = std::make_shared<MeanSquaredError>();

    std::vector<std::pair<Vector<float>, Vector<float>>> train_set_, test_set_;

public:

    BP() {  }

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
            pre_layout->weight = Tensor<float, 2>(pre_layout->size, layout->size);
            random_tensor(pre_layout->weight, -1, 1);
        }
        layouts_.push_back(std::move(layout));

    }

    void forward(const Vector<float> &in) {
        auto in_layout = layouts_.front();
        in_layout->forward(in);
        for (int i = 1;i < layouts_.size(); ++i)
            layouts_[i]->forward(layouts_[i-1]->next_in);
    }

    void backward(const Vector<float> &out) {
        auto out_layout = layouts_.back();
        Vector<float> gradient(out.size());
        for (int i = 0;i < gradient.size(); ++i)
            gradient[i] = loss_->deaction(out_layout->out[i], out[i]);

        out_layout->backward(gradient);

        for (int i = layouts_.size() - 2; i > 0; --i)
            layouts_[i]->backward(out_layout->gradient);
    }

    void train(const Vector<float> &in, const Vector<float> &out) {
        forward(in);
        backward(out);
    }


    void train(const std::vector<std::pair<Vector<float>, Vector<float>>> &dataset,
                float train_proportion, size_t train_count, int seed, bool updata_acc = false) {

        ::train_count = train_count;

        int train_size = dataset.size() * train_proportion;

        train_set_ = std::vector(dataset.begin(), dataset.begin() + train_size);
        test_set_ = std::vector(dataset.begin() + train_size, dataset.end());

        // 随机打乱训练集
        int count = 1000;
        std::default_random_engine engine(seed);
        std::uniform_int_distribution<> dis(0, train_size - 1);
        for (int i = 0; i < count; ++i)
            std::swap(train_set_[dis(engine)], train_set_[dis(engine)]);

        Console::hind_cursor();
        for (int i = 0;i < train_count; ++i) {
            cur_train_count = i;

            // 开始训练
            for (auto &[in, out] : train_set_)
                train(in, out);

            // 开始测试
            if (updata_acc) {
                float error{};
                for (auto &[in, out] : test_set_) {
                    forward(in);
                    error += mean_relative_error(layouts_.back()->out, out);
                }
                error /= test_set_.size();

                std::cout << std::format("{}/{} {:.2f}% , error is {:.6f}", i, train_count, i * 100.f / train_count, error) << '\r';
                std::fflush(stdout);
            }
            else {
                std::cout << std::format("{}/{} {:.2f}%", i, train_count, i * 100.f / train_count) << '\r';
                std::fflush(stdout);
            }
        }
        Console::show_cursor();

        if (!updata_acc) {
            float error{};
            for (auto &[in, out] : test_set_) {
                forward(in);
                error += mean_relative_error(layouts_.back()->out, out);
            }
            error /= test_set_.size();

            std::cout << std::format("error is {:.6f}", error) << std::endl;
        }
    }

    const Vector<float>& forecast(Vector<float> &in) const {
        layouts_.front()->forward(in);
        for (int i = 1;i < in.size(); ++i)
            layouts_[i]->forward(layouts_[i-1]->next_in);

        return layouts_.back()->out;
    }

    void set_optimizer(std::shared_ptr<Optimizer> optimizer) {
        ::optimizer = optimizer;
    }
};