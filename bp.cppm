/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/06 22:51
*******************************************************************************/
export module modforge.deep_learning.bp;

import std;
import modforge.tensor;
import modforge.deep_learning.tools;
import modforge.average_queue;
import modforge.console;

int cur_train_count;
int train_count;
std::shared_ptr<Optimizer> optimizer;

struct Layer {
    std::size_t size;
    Vector<float> in, out, next_in{}, gradient{};
    Tensor<float, 2> weight{};
    bool have_next{};

    std::shared_ptr<Activate> action;


    Layer(const std::size_t size, const std::shared_ptr<Activate> &action = std::make_shared<Relu>())
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
            if (optimizer)
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
    std::vector<std::shared_ptr<Layer>> layers_;
    std::shared_ptr<LossFunction> loss_ = std::make_shared<MeanSquaredError>();

    std::vector<std::pair<Vector<float>, Vector<float>>> train_set_, test_set_;

public:

    BP() {  }

    template <typename ... Args>
    BP(Args &&... args) requires (sizeof ...(args) >= 2 && (std::is_same_v<Args, int> && ...)) {
        for (auto size : {args...})
            add_layer(size);
    }


    void add_layer(std::size_t n, const std::shared_ptr<Activate> &action = std::make_shared<Relu>()) {
        auto layer = std::make_shared<Layer>(n, action);

        if (!layers_.empty()) {
            auto pre_layer = layers_.back();
            pre_layer->have_next = true;
            pre_layer->weight = Tensor<float, 2>(pre_layer->size, layer->size);
            random_tensor(pre_layer->weight, -1, 1);
        }
        layers_.push_back(std::move(layer));
    }

    void forward(const Vector<float> &in) {
        auto in_layer = layers_.front();
        in_layer->forward(in);
        for (int i = 1;i < layers_.size(); ++i)
            layers_[i]->forward(layers_[i-1]->next_in);
    }

    void backward(const Vector<float> &out) {
        auto out_layer = layers_.back();
        Vector<float> gradient(out.size());
        for (int i = 0;i < gradient.size(); ++i)
            gradient[i] = loss_->deaction(out_layer->out[i], out[i]);

        out_layer->backward(gradient);

        for (int i = layers_.size() - 2; i >= 0; --i)
            layers_[i]->backward(layers_[i+1]->gradient);
    }

    void train(const Vector<float> &in, const Vector<float> &out) {
        forward(in);
        backward(out);
    }


    void train(const std::vector<std::pair<Vector<float>, Vector<float>>> &dataset,
                float train_proportion, std::size_t train_count, int seed, bool updata_acc = false) {

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

            std::cout << std::format("{}/{} {}%", i, train_count, i * 100.f / train_count);
            if (updata_acc) {
                float error{};
                for (auto &[in, out] : test_set_) {
                    forward(in);
                    error += mean_relative_error(layers_.back()->out, out);
                }
                error /= test_set_.size();
                std::cout << std::format(" , error is {:.6f}", error);
            }
            std::cout << '\r';
            std::cout.flush();
        }
        Console::show_cursor();
        std::endl(std::cout);
    }

    const Vector<float>& forecast(Vector<float> &in) const {
        layers_.front()->forward(in);
        for (int i = 1;i < layers_.size(); ++i)
            layers_[i]->forward(layers_[i-1]->next_in);

        return layers_.back()->out;
    }

    void set_optimizer(std::shared_ptr<Optimizer> optimizer) {
        ::optimizer = optimizer;
    }
};

/****************************** 用例 ******************************
#include <iostream>

import modforge.tensor;
import modforge.deep_learning.bp;
import modforge.deep_learning.tools;

auto CreateDataset() {
    std::vector<std::pair<Vector<float>, Vector<float>>> ret;

    for (int i = 0;i < 100000; ++i) {
        Vector<float> in(2);
        Vector<float> out(1);
        in[0] = GetRandom(0, 0.4);
        in[1] = GetRandom(0, 0.4);
        out[0] = in[0] + in[1];

        ret.push_back(std::make_pair(in, out));
    }
    return ret;
}

int main() {

    auto data = CreateDataset();

    BP bp(2, 100, 1);
    bp.set_optimizer(std::shared_ptr<Optimizer>(new CosineAnnealing(0.01, 0.0001, 1000 * 0.5)));
    bp.train(data, 0.7, 1000, 0, true);

    Vector<float> in(2);
    in[0] = 0.2;
    in[1] = 0.1;
    std::cout << bp.forecast(in)[0] << std::endl;
    in[0] = 0.2;
    in[1] = 0.3;
    std::cout << bp.forecast(in)[0] << std::endl;

    return 0;
}
*******************************************************************************/
