import std;
import modforge;

int test_deep_learning() {
    namespace dl = modforge::deep_learning;

    // ---- Vector * Tensor ----
    {
        modforge::Vector<int> v(3);
        v[0] = 1; v[1] = 2; v[2] = 3;

        modforge::Tensor<int, 2> m(3, 2);
        m[0, 0] = 1; m[0, 1] = 2;
        m[1, 0] = 3; m[1, 1] = 4;
        m[2, 0] = 5; m[2, 1] = 6;

        auto out = v * m;
        if (out.size() != 2 || out[0] != 22 || out[1] != 28)
            return __LINE__;
    }

    // ---- Tensor 加减乘 ----
    {
        modforge::Tensor<int, 2> a(2, 2);
        modforge::Tensor<int, 2> b(2, 2);
        a[0,0]=1; a[0,1]=2; a[1,0]=3; a[1,1]=4;
        b[0,0]=5; b[0,1]=6; b[1,0]=7; b[1,1]=8;

        auto c = a + b;
        if (c[0,0] != 6 || c[1,1] != 12)
            return __LINE__;

        c -= b;
        if (c[0,0] != 1 || c[1,1] != 4)
            return __LINE__;

        auto d = a * b;
        if (d[0,0] != 19 || d[0,1] != 22 || d[1,0] != 43 || d[1,1] != 50)
            return __LINE__;
    }

    // ---- 转置 ----
    {
        modforge::Tensor<int, 2> t(2, 3);
        int val = 1;
        t.foreach([&](int& x){ x = val++; });

        t.transpose();
        if (t.extent(0) != 3 || t.extent(1) != 2)
            return __LINE__;
        if (t[0,0] != 1 || t[0,1] != 4 || t[2,1] != 6)
            return __LINE__;
    }

    // ---- 序列化 round-trip ----
    {
        modforge::Tensor<int, 2> src(2, 2);
        src[0,0]=9; src[0,1]=8; src[1,0]=7; src[1,1]=6;

        std::stringstream ss;
        ss << src;

        modforge::Tensor<int, 2> dst;
        ss >> dst;
        if (dst.extent(0) != 2 || dst.extent(1) != 2)
            return __LINE__;
        if (dst[0,0] != 9 || dst[1,1] != 6)
            return __LINE__;
    }

    // ---- 尺寸不匹配抛异常 ----
    {
        bool thrown = false;
        try {
            modforge::Tensor<int, 2> a(2, 3);
            modforge::Tensor<int, 2> b(4, 2);
            (void)(a + b);
        } catch (const std::runtime_error&) {
            thrown = true;
        }
        if (!thrown)
            return __LINE__;
    }

    // ---- 激活函数工具（concept 版） ----
    if (dl::relu(-3) != 0 || dl::relu(5) != 5)
        return __LINE__;

    const double sig0 = dl::sigmoid(0.0);
    if (std::abs(sig0 - 0.5) > 1e-12)
        return __LINE__;

    const std::vector<double> logits{1.0, 2.0, 3.0};
    const auto probs = dl::softmax(logits);
    if (probs.size() != logits.size())
        return __LINE__;

    const double sum = std::accumulate(probs.begin(), probs.end(), 0.0);
    if (std::abs(sum - 1.0) > 1e-12)
        return __LINE__;
    if (!(probs[0] < probs[1] && probs[1] < probs[2]))
        return __LINE__;

    const std::vector<double> large{1000.0, 1001.0};
    const auto stable = dl::softmax(large);
    if (stable.size() != 2 || !std::isfinite(stable[0]) || !std::isfinite(stable[1]))
        return __LINE__;

    if (dl::argmax(std::vector<int>{1, 7, 3}) != 1)
        return __LINE__;

    bool thrown = false;
    try {
        (void)dl::argmax(std::vector<int>{});
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    if (!thrown)
        return __LINE__;

    // ---- 类版激活/损失/优化器 ----
    {
        modforge::Sigmoid sigmoid;
        if (std::abs(sigmoid.action(0.0) - 0.5) > 1e-12)
            return __LINE__;

        modforge::MeanSquaredError mse;
        if (std::abs(mse.deaction(0.7, 0.2) - 0.5) > 1e-12)
            return __LINE__;

        modforge::CosineAnnealing anneal(0.01, 0.0001, 100);
        if (std::abs(anneal.get_speed(0, 100) - 0.01) > 1e-9)
            return __LINE__;
        if (std::abs(anneal.get_speed(50, 100) - 0.00505) > 1e-9)
            return __LINE__;
    }

    // ---- OneHot ----
    {
        auto onehot = modforge::OneHot::type_to_onehot<4>(2);
        if (onehot.size() != 4 || onehot[2] != 1 || onehot[0] != 0)
            return __LINE__;
        if (modforge::OneHot::out_to_type(onehot) != 2)
            return __LINE__;
    }

    // ---- BP 网络小样本收敛 ----
    // 必须先 seed_random：数据集与权重初始化都走全局 GetRandom（random_device 播种），
    // 不固定种子则每次运行样本都不同，收敛断言会随机失败。
    {
        modforge::seed_random(42);

        std::vector<std::pair<modforge::Vector<float>, modforge::Vector<float>>> dataset;
        for (int i = 0; i < 400; ++i) {
            modforge::Vector<float> in(2), out(1);
            in[0] = modforge::GetRandom(0.f, 0.4f);
            in[1] = modforge::GetRandom(0.f, 0.4f);
            out[0] = in[0] + in[1];
            dataset.emplace_back(std::move(in), std::move(out));
        }

        modforge::BP bp(2, 32, 1);
        bp.train(dataset, 0.7f, 300, 42);

        modforge::Vector<float> in(2);
        in[0] = 0.2f; in[1] = 0.1f;
        const float predicted = bp.forecast(in)[0];
        if (std::abs(predicted - 0.3f) > 0.05f)
            return __LINE__;
    }

    return 0;
}
