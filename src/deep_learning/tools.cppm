/*******************************************************************************
 * @Author : hexne
 * @Data   : 2025/03/26 22:21
*******************************************************************************/

module;
export module modforge.deep_learning.tools;

import modforge.tensor;
import std;
#ifdef ENABLE

export NAMESPACE_BEGIN

/**************************************** 激活函数 ****************************************/
/** @brief 激活函数接口：action 为前向，deaction 为导数 */
struct Activate {
    /** @brief 前向激活
     *  @param num 激活前的值
     *  @return 激活后的值
     */
    virtual double action(double num) = 0;
    /** @brief 激活函数的导数
     *  @param num 激活前的值（不是激活后的值）
     *  @return 导数值
     */
    virtual double deaction(double num) = 0;
    virtual ~Activate() = default;
};

/** @brief 激活函数类型，用于序列化 */
enum class ActivateType : unsigned char {
    Sigmoid, Relu
};

/** @brief 二进制写出激活函数类型 */
std::ostream & operator << (std::ostream &out, const ActivateType &val) {
    out.write(reinterpret_cast<const char *>(&val), sizeof(ActivateType));
    return out;
}
/** @brief 二进制读入激活函数类型 */
std::istream & operator >> (std::istream &in, ActivateType &val) {
    in.read(reinterpret_cast<char *>(&val), sizeof(ActivateType));
    return in;
}

/** @brief Sigmoid 激活：1 / (1 + e^-x) */
struct Sigmoid : Activate {
    /** @brief Sigmoid 前向 */
    double action(double num) override {
        return 1.0 / (1.0 + std::exp(-num));
    }

    /** @brief Sigmoid 导数：s * (1 - s)，s 为激活后的值 */
    double deaction(double num) override {
        double sigmod = action(num);
        return sigmod * (1.0 - sigmod);
    }
};

/** @brief ReLU 激活：max(0, x) */
struct Relu : Activate {

    /** @brief ReLU 前向 */
    double action(double num) override {
        return std::max(0.0, num);
    }

    /** @brief ReLU 导数：x > 0 时为 1，否则为 0 */
    double deaction(double num) override {
        if (num > 0)
            return 1;
        return 0;
    }

};



/**************************************** 损失函数 ****************************************/
/** @brief 损失函数接口，同时提供标量版与向量版 */
struct LossFunction {
    /** @brief 单值损失
     *  @param predicted_value 预测值
     *  @param true_value 真实值
     *  @return 损失值
     */
    virtual double action(double predicted_value, double true_value) = 0;
    /** @brief 单值损失对预测值的导数
     */
    virtual double deaction(double predicted_value, double true_value) = 0;

    /** @brief 向量版损失：逐分量计算
     *  @throw std::invalid_argument 两向量长度不等
     */
    virtual Vector<float> action(Vector<float> predicted_value, Vector<float> true_value) = 0;
    /** @brief 向量版损失的导数：逐分量计算
     *  @throw std::invalid_argument 两向量长度不等
     */
    virtual Vector<float> deaction(Vector<float> predicted_value, Vector<float> true_value) = 0;

    virtual ~LossFunction() = default;
};

/** @brief 均方误差：0.5 * (真实值 - 预测值)^2，导数为 预测值 - 真实值 */
struct MeanSquaredError : LossFunction {
    // 参数分别是 预测值 ， 真实值
    double action(double predicted_value, double true_value) override {
        return 0.5 * std::pow(true_value - predicted_value, 2);
    }
    double deaction(double predicted_value, double true_value) override {
        return predicted_value - true_value;
    }

    Vector<float> action(Vector<float> predicted_value, Vector<float> true_value) override {
        if (predicted_value.size() != true_value.size())
            throw std::invalid_argument("predicted_value.size() != true_value.size()");

        Vector<float> ret(predicted_value.size());
        for (int i = 0;i < predicted_value.size(); ++i)
            ret[i] = action(predicted_value[i], true_value[i]);

        return ret;
    }

    Vector<float> deaction(Vector<float> predicted_value, Vector<float> true_value) override {
        if (predicted_value.size() != true_value.size())
            throw std::invalid_argument("predicted_value.size() != true_value.size()");

        Vector<float> ret(predicted_value.size());
        for (int i = 0;i < predicted_value.size(); ++i)
            ret[i] = deaction(predicted_value[i], true_value[i]);

        return ret;
    }
};


/****************************** 优化器 ******************************/
/** @brief 优化器接口：按训练进度给出学习率 */
class Optimizer {
public:
    /** @brief 当前学习率
     *  @param cur 当前迭代序号，从 0 开始
     *  @param count 总迭代次数
     *  @return 学习率
     */
    virtual double get_speed(int cur, int count) = 0;
    virtual ~Optimizer() = default;
};

/** @brief 余弦退火：学习率在 [min_speed, max_speed] 间按余弦周期衰减 */
class CosineAnnealing : public Optimizer {
    double max_speed;
    double min_speed;
    int T_max;

public:
    /** @brief 构造余弦退火优化器
     *  @param max_speed 峰值学习率
     *  @param min_speed 谷值学习率
     *  @param T_max 退火周期（迭代次数）
     */
    CosineAnnealing(double max_speed, double min_speed, int T_max)
        : max_speed(max_speed), min_speed(min_speed), T_max(T_max) {}

    /** @brief 按 cur % T_max 的进度计算学习率
     *  @param cur 当前迭代序号
     *  @param count 总迭代次数，本实现未使用
     *  @return 学习率
     */
    double get_speed(int cur, int count) override {
        // 计算当前进度比例 (0-1)
        double progress = static_cast<double>(cur % T_max) / T_max;

        // 应用余弦退火公式
        return min_speed + 0.5 * (max_speed - min_speed) *
               (1 + std::cos(progress * std::numbers::pi));
    }
};


/**************************************** 归一化 ****************************************/
/** @brief 归一化接口，当前无实现 */
struct Normalization {
    virtual double action(double num) = 0;
    virtual double deaction(double num) = 0;
    virtual ~Normalization() = default;
};

/****************************** 获取随机值 ******************************/
/** @brief 全局随机引擎，默认由 random_device 播种 */
inline std::mt19937 &random_engine() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

/** @brief 固定全局随机引擎的种子，使后续 GetRandom / random_tensor 结果可复现
 *  @param seed 种子值（不调用则每次运行都不同）
 */
inline void seed_random(std::uint32_t seed) {
    random_engine().seed(seed);
}

/** @brief 取 [min, max) 区间的随机实数，使用全局随机引擎
 *  @param min 下界（含）
 *  @param max 上界（不含）
 *  @note 需要可复现结果时先调用 seed_random()
 */
double GetRandom(const double min, const double max) {
    std::uniform_real_distribution<> dis(min, max);
    return dis(random_engine());
}
template<typename T, std::size_t Extent = 2>
/** @brief 用 [min, max) 的随机值填充张量
 *  @param tensor 目标张量：按值传入，依赖 Tensor 的浅拷贝语义使修改对调用方可见
 *  @param min 下界（含）
 *  @param max 上界（不含）
 */
void random_tensor(Tensor<T, Extent>tensor, double min, double max) {
    tensor.foreach([&](T &val) {
        val = GetRandom(min, max);
    });

}

/** @brief One-Hot 编码与解码 */
namespace OneHot {
    // type 从0开始
    /** @brief 取最大分量的下标作为类别（argmax）
     *  @param out 网络输出，须非空
     *  @return 类别编号，从 0 开始
     */
    int out_to_type(const Vector<float> &out) {
        int pos = 0;
        float max = out[0];
        for (int i = 1; i < out.size(); ++i) {
            if (out[i] > max) {
                max = out[i];
                pos = i;
            }
        }
        return pos;
    }

    template<std::size_t N>
    /** @brief 类别编号转 One-Hot 向量
     *  @tparam N 类别总数
     *  @param type 类别编号，从 0 开始
     *  @return 长度 N 的向量，type 位为 1、其余为 0
     */
    Vector<float> type_to_onehot(int type) {
        Vector<float> ret(N);
        for (int i = 0; i < N; ++i)
            ret[i] = 0.f;
        ret[type] = 1;
        return ret;
    }

}



/**************************************** 常用数值工具（concept 版） ****************************************/
/** @brief 与张量无关的通用数值工具 */
namespace deep_learning {
    template <typename T>
    /** @brief 数值类型约束 */
    concept Numeric = std::is_arithmetic_v<T>;

    template <Numeric T>
    /** @brief ReLU：max(0, value) */
    T relu(T value) {
        return std::max(T{}, value);
    }

    template <Numeric T>
    /** @brief Sigmoid：1 / (1 + e^-value) */
    T sigmoid(T value) {
        return T{1} / (T{1} + std::exp(-value));
    }

    template <Numeric T>
    /** @brief 最大值所在的下标
     *  @throw std::invalid_argument 输入为空
     */
    std::size_t argmax(const std::vector<T>& values) {
        if (values.empty())
            throw std::invalid_argument("argmax requires a non-empty vector");

        return static_cast<std::size_t>(std::distance(values.begin(), std::max_element(values.begin(), values.end())));
    }

    template <Numeric T>
    /** @brief softmax 归一化：先减最大值再取指数，避免上溢
     *  @return 概率分布；输入为空时返回空
     */
    std::vector<T> softmax(const std::vector<T>& values) {
        if (values.empty())
            return {};

        const T max_value = *std::max_element(values.begin(), values.end());
        std::vector<T> result;
        result.reserve(values.size());

        T sum{};
        for (const auto& value : values) {
            const T exp_value = std::exp(value - max_value);
            result.push_back(exp_value);
            sum += exp_value;
        }

        for (auto& value : result)
            value /= sum;

        return result;
    }
}
NAMESPACE_END
#endif