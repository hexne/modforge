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

// export
// struct Layout {
//
//     size_t size;
//     Matrix<float> in, out, next_in;
//     Matrix<float> weight, gradient;
//     bool have_next{};
//
//     std::shared_ptr<Activation> action;
//
//
//     explicit Layout(const size_t size, const std::shared_ptr<Activation> &action = std::make_shared<Sigmoid>())
//             : size(size), in(1, size), out(1, size) , action(action) {  }
//
//
//     void forward(const Matrix<float> &pre_in) {
//         in = pre_in;
//
//         // 直接应用激活函数，没有应用线性变换
//         for (int i = 0;i < in.y; ++i)
//             out[0, i] = action->action(in[0, i]);
//
//         if (have_next)
//             next_in = out * weight;
//     }
//
//     void backward(const Matrix<float> &next_gradient) {
//         if (!have_next)
//             gradient = next_gradient;
//         else
//             gradient = weight * next_gradient;
//
//         // 下一层要用的梯度
//         for (int i = 0;i < gradient.y; ++i)
//             gradient[0, i] *= action->deaction(in[0, i]);
//
//         // 如果是最后一层，只需要求出梯度即可
//         if (!have_next)
//             return;
//
//         constexpr float speed = 0.001;
//
//         for (int i = 0;i < weight.x; ++i) {
//             for (int j = 0;j < weight.y; ++j) {
//                 weight[i, j] -= speed * gradient[0, j] * out[0, i];
//             }
//         }
//
//     }
//
//
// };
//
// export
// struct BP {
//     std::vector<std::shared_ptr<Layout>> layouts_;
//     std::shared_ptr<LossFunction> loss_ = std::make_shared<MeanSquaredError>();
//     float speed = 0.001;
//
//     std::vector<std::pair<std::initializer_list<float>, std::initializer_list<float>>> train_set_, test_set_;
//
// public:
//
//     BP() {
//
//     }
//
//
//     template <typename ... Args>
//     BP(Args &&... args) requires (sizeof ...(args) >= 2 && (std::is_same_v<Args, int> && ...)) {
//         for (auto size : {args...})
//             add_layout(size);
//     }
//
//     void add_layout(size_t size) {
//         add_layout(std::make_shared<Layout>(size));
//     }
//     void add_layout(Layout *layout) {
//         add_layout(std::shared_ptr<Layout>(layout));
//     }
//     void add_layout(std::shared_ptr<Layout> layout) {
//
//         if (!layouts_.empty()) {
//             auto pre_layout = layouts_.back();
//             pre_layout->have_next = true;
//             pre_layout->weight = Matrix<float>(pre_layout->size, layout->size);
//             pre_layout->weight.random_init();
//         }
//         layouts_.push_back(std::move(layout));
//
//     }
//
//     void forward(const std::initializer_list<float> &in, const std::initializer_list<float> &out) {
//
//         auto in_layout = layouts_.front();
//         in_layout->forward(in);
//         for (int i = 1;i < layouts_.size(); ++i)
//             layouts_[i]->forward(layouts_[i-1]->next_in);
//
//     }
//
//     void train(const std::initializer_list<float> &in, const std::initializer_list<float> &out) {
//
//         // 前向传播
//         auto in_layout = layouts_.front();
//         in_layout->forward(in);
//         for (int i = 1;i < layouts_.size(); ++i)
//             layouts_[i]->forward(layouts_[i-1]->next_in);
//
//
//         // 反向传播
//         auto out_layout = layouts_.back();
//         Matrix<float> predicted(out);
//         out_layout->backward(loss_->deaction(out_layout->out, predicted));
//         for (int i = layouts_.size() - 2;i > 0; --i)
//             layouts_[i]->backward(out_layout->gradient);
//     }
//     void train(std::vector<std::pair<std::initializer_list<float> , std::initializer_list<float>>>dataset,
//         float train_proportion, size_t train_count, int seed) {
//
//         int train_size = dataset.size() * train_proportion;
//
//
//         train_set_ = std::vector(dataset.begin(), dataset.begin() + train_size);
//         test_set_ = std::vector(dataset.begin() + train_size, dataset.end());
//
//         // 随机打乱训练集
//         int count = 1000;
//         std::default_random_engine engine(seed);
//         std::uniform_int_distribution<> dis(0, train_size - 1);
//         for (int i = 0; i < count; ++i)
//             std::swap(train_set_[dis(engine)], train_set_[dis(engine)]);
//
//         // 开始训练
//
//         AverageQueue<float, 20> aver_que;
//         for (int i = 0;i < train_count; ++i) {
//             float acc{};
//             for (const auto &[in, res] : train_set_) {
//                 // 训练一对数据
//                 train(in, res);
//
//                 float acc{};
//                 std::vector res_vec = res;
//                 std::vector<float> forward_vec = layouts_.back()->out;
//                 for (int _ = 0; _ < res_vec.size(); ++_) {
//                     acc += std::abs(res_vec[_] - forward_vec[_]) / res_vec[_];
//                 }
//             }
//             aver_que.push_back(acc / train_count);
//             std::cout << i << " / " << train_count << " , acc is : " << aver_que.get_average() << "\r";
//         }
//
//     }
//
// };