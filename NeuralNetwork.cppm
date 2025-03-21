/*******************************************************************************
 * @Author : yongheng
 * @Data   : 2024/12/06 22:51
*******************************************************************************/

module;
#include <array>
#include <iostream>
#include <memory>
#include <ostream>
#include <vector>
#include <random>
#include <tuple>
#include <type_traits>

#include "tools.h"
export module NeuralNetwork;

import Random;
import Matrix;

export
NAMESPACE_BEGIN(nl)

template <typename T = float>
struct Activation {
    virtual T action(T num) = 0;
    virtual T deaction(T num) = 0;

    virtual ~Activation() = default;
};

template <typename T>
struct Sigmod : Activation<T> {
    T action(T num) override {
        return 1.0 / (1.0 + exp(-num));
    }

    T deaction(T num) override {
        T sigmod = 1.0 + exp(-num);
        return sigmod * (1.0 - sigmod);
    }
};


template <typename T = float>
struct Layout {
    std::vector<T> in{}, out{}, res{};
    nl::Matrix<T> gradient{}, weight{};
    std::shared_ptr<Activation<T>> action;

    Layout(size_t size, Activation<T> *p = new Sigmod<T>())
            : in(std::vector<T>(size)), out(std::vector<T>(size)),
                action(std::shared_ptr<Activation<T>>(p))
    {
        for (auto &num : in)
            num = nl::Random::get_random_float(0.f, 1.f);

        for (auto &num : out)
            num = nl::Random::get_random_float(0.f, 1.f);
    }
    Layout (const Layout &&layout) {
        in = layout.in;
        out = layout.out;
        weight = layout.weight;
        res = layout.res;
        gradient = layout.gradient;
    }

    // 前向传播
    void forward() {
        for (int i = 0;i < in.size(); ++i) {
            // out[i] = action->action(in[i]);
            out[i] = in[i];
        }

        res = std::vector<T> (weight.x);
        for (int i = 0;i < res.size(); ++i) {
            T sum{};
            for (int j = 0;j < out.size(); ++j) {
                sum += out[j] * weight[i, j];
            }
            res[i] = sum;
        }

    }

    // 反向传播
    void backward() {

    }

    void update_weight() {

    }

    size_t size() {
        return in.size();
    }
};



template <typename T = float>
class Backpropagation {
public:
    // @TODO test
    std::vector<Layout<T>> layout;


    Backpropagation() {

    }

    void add_hind_layout(size_t size) {
        if (!layout.empty()) {
            auto &prev_layout = layout.back();
            size_t prev_size = prev_layout.size();
            prev_layout.gradient = nl::Matrix<T>(size, prev_size);
            prev_layout.weight = nl::Matrix<T>(size, prev_size);
        }
        layout.push_back(Layout<T>(size));
    }
    void forward() {


    }

};


NAMESPACE_END(nl)

