/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/06 22:51
 *
 *  从 modforge_back 的 deep_learning 模块迁移而来。
 *  迁移改动：训练计数与优化器由模块级全局变量收进 BP 成员，
 *  Layer::backward 改为显式接收学习速度，去除对 average_queue 的空依赖。
*******************************************************************************/
export module modforge.deep_learning.bp;

import std;
import modforge.tensor;
import modforge.deep_learning.tools;
import modforge.terminal;

#ifdef ENABLE
NAMESPACE_BEGIN

/** @brief 全连接层：持有输入 / 输出 / 梯度，以及到下一层的权重 */
struct Layer {
    std::size_t size;
    Vector<float> in, out, next_in{}, gradient{};
    Tensor<float, 2> weight{};
    bool have_next{};

    std::shared_ptr<Activate> action;


    /** @brief 构造一层
     *  @param size 神经元个数
     *  @param action 激活函数，默认 ReLU
     */
    Layer(const std::size_t size, const std::shared_ptr<Activate> &action = std::make_shared<Relu>())
        : size(size), in(size), out(size) , action(action) {  }

    /** @brief 前向：先激活，若有下一层则计算 next_in = out * weight
     *  @param pre_in 本层输入，长度须等于 size
     */
    void forward(const Vector<float> &pre_in) {
        in = pre_in;

        for (int i = 0;i < in.size(); ++i)
            out[i] = action->action(in[i]);

        if (have_next)
            next_in = out * weight;
    }
    /** @brief 反向：更新到下一层的权重，并把梯度传回上一层
     *  @param next_gradient 来自下一层的梯度
     *  @param speed 学习率
     *  @note 返回后 gradient 即为传给上一层的梯度
     */
    void backward(const Vector<float> &next_gradient, double speed) {
        gradient = next_gradient;

        // 如果不是最后一层
        if (have_next) {
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
/** @brief 平均相对误差，跳过 target 为 0 的分量以避免除零
 *  @return 误差均值；无有效分量时返回 0.0
 */
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

/** @brief 多层全连接网络，以反向传播训练 */
export
class BP {
    std::vector<std::shared_ptr<Layer>> layers_;
    std::shared_ptr<LossFunction> loss_ = std::make_shared<MeanSquaredError>();

    std::vector<std::pair<Vector<float>, Vector<float>>> train_set_, test_set_;

    int cur_train_count_{};
    int train_count_{};
    std::shared_ptr<Optimizer> optimizer_;

    [[nodiscard]] double speed() const {
        if (!optimizer_)
            return 0.001;
        return optimizer_->get_speed(cur_train_count_, train_count_);
    }

public:

    /** @brief 默认构造：不含任何层，需随后调用 add_layer */
    BP() {  }

    template <typename ... Args>
    /** @brief 依次把 args 中每个整数作为一层的神经元数
     *  @param args 各层神经元数，至少 2 个且须为 int
     */
    BP(Args &&... args) requires (sizeof ...(args) >= 2 && (std::is_same_v<Args, int> && ...)) {
        for (auto size : {args...})
            add_layer(size);
    }


    /** @brief 追加一层；若已有层，则为上一层创建并用 [-1, 1) 随机初始化权重
     *  @param n 该层神经元个数
     *  @param action 激活函数，默认 ReLU
     *  @note 权重随机化使用全局随机引擎，需可复现时先调用 seed_random()
     */
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

    /** @brief 前向传播，各层结果存于该层的 out */
    void forward(const Vector<float> &in) {
        auto in_layer = layers_.front();
        in_layer->forward(in);
        for (int i = 1;i < layers_.size(); ++i)
            layers_[i]->forward(layers_[i-1]->next_in);
    }

    /** @brief 反向传播：以损失函数的导数起算，逐层回传并更新权重
     *  @param out 期望输出
     */
    void backward(const Vector<float> &out) {
        auto out_layer = layers_.back();
        Vector<float> gradient(out.size());
        for (int i = 0;i < gradient.size(); ++i)
            gradient[i] = loss_->deaction(out_layer->out[i], out[i]);

        out_layer->backward(gradient, speed());

        for (int i = layers_.size() - 2; i >= 0; --i)
            layers_[i]->backward(layers_[i+1]->gradient, speed());
    }

    /** @brief 单样本训练：一次前向加一次反向 */
    void train(const Vector<float> &in, const Vector<float> &out) {
        forward(in);
        backward(out);
    }


    /** @brief 批量训练：按比例切分数据集后训练 train_count 轮
     *  @param dataset 完整数据集
     *  @param train_proportion 训练集占比，取前 n 条为训练集、其余为测试集
     *  @param train_count 训练轮数
     *  @param seed 打乱训练集的随机种子
     *  @param updata_acc 是否每轮在测试集上计算并打印平均相对误差
     *  @note train_proportion 为 1 时测试集为空，开启 updata_acc 会除零
     */
    void train(const std::vector<std::pair<Vector<float>, Vector<float>>> &dataset,
                float train_proportion, std::size_t train_count, int seed, bool updata_acc = false) {

        train_count_ = train_count;

        int train_size = dataset.size() * train_proportion;

        train_set_ = std::vector(dataset.begin(), dataset.begin() + train_size);
        test_set_ = std::vector(dataset.begin() + train_size, dataset.end());

        // 随机打乱训练集
        int count = 1000;
        std::default_random_engine engine(seed);
        std::uniform_int_distribution<> dis(0, train_size - 1);
        for (int i = 0; i < count; ++i)
            std::swap(train_set_[dis(engine)], train_set_[dis(engine)]);

        terminal::hide_cursor();
        for (int i = 0;i < train_count; ++i) {
            cur_train_count_ = i;

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
        terminal::show_cursor();
        std::endl(std::cout);
    }

    /** @brief 推理：前向传播并返回输出层结果
     *  @return 输出层 out 的引用，下次前向即被覆盖
     */
    const Vector<float>& forecast(Vector<float> &in) const {
        layers_.front()->forward(in);
        for (int i = 1;i < layers_.size(); ++i)
            layers_[i]->forward(layers_[i-1]->next_in);

        return layers_.back()->out;
    }

    /** @brief 设置优化器；未设置时学习率固定为 0.001
     */
    void set_optimizer(std::shared_ptr<Optimizer> optimizer) {
        optimizer_ = std::move(optimizer);
    }
};

NAMESPACE_END

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

#endif