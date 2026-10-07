/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
export module modforge.deep_learning.cnn;

import std;
import modforge.tensor;
import modforge.deep_learning.tools;

#ifdef ENABLE
NAMESPACE_BEGIN
/** @brief 类型别名：标签 / 卷积核 / 全连接权重 / 特征图 / 池化窗口 */
using Label      = Vector<float>;
using Kernels    = Tensor<float, 4>;
using Weights    = Tensor<float, 4>;
using FeatureMap = Tensor<float, 3>;
using PoolWindow = Tensor<float, 2>;

/** @brief 特征图尺寸：x 为宽、y 为高、z 为通道数 */
struct FeatureExtent {
    int x{}, y{}, z{};
    void read(std::istream &in) {
        in.read(reinterpret_cast<char*>(&z), sizeof(z));
        in.read(reinterpret_cast<char*>(&x), sizeof(x));
        in.read(reinterpret_cast<char*>(&y), sizeof(y));
    }
    void write(std::ostream &out) const {
        out.write(reinterpret_cast<const char*>(&z), sizeof(z));
        out.write(reinterpret_cast<const char*>(&x), sizeof(x));
        out.write(reinterpret_cast<const char*>(&y), sizeof(y));
    }
    friend std::istream &operator >> (std::istream &in, FeatureExtent &extent) {
        extent.read(in);
        return in;
    }
    friend std::ostream &operator << (std::ostream &out, const FeatureExtent &extent) {
        extent.write(out);
        return out;
    }
};

/** @brief 一条样本：输入特征图 + 标签向量 */
export struct Data {
    FeatureMap data;
    Label label;
};

/** @brief 池化区域内最大值的坐标，供反向传播回传梯度 */
export struct PoolMaxPos {
    int x;
    int y;
};
using PoolLayerMaxPos = Tensor<PoolMaxPos, 3>;


/** @brief 训练超参：学习率 / 动量 / 权重衰减 */
#define LEARNING_RATE 0.01
#define MOMENTUM 0.6
#define WEIGHT_DECAY 0.001

/** @brief 带动量与权重衰减的权重更新
 *  @param w 待更新的权重
 *  @param gradient 当前梯度
 *  @param old_gradient 动量项，由 update_gradient 维护
 *  @param multp 梯度的额外缩放系数，默认 1
 */
void update_weight(float& w, float gradient, float old_gradient, float multp = 1) {
    w -= LEARNING_RATE * (gradient + old_gradient * MOMENTUM) * multp + LEARNING_RATE * WEIGHT_DECAY * w;
}

/** @brief 更新动量项：old_gradient = (gradient + old_gradient) * MOMENTUM
 */
void update_gradient(float &gradient, float &old_gradient) {
    old_gradient = (gradient + old_gradient) * MOMENTUM;
}

/** @brief 层类型，用于序列化 */
enum class LayerType : unsigned char {
    None, Conv, Activate, Pool, FC
};
std::ostream & operator << (std::ostream &out, const LayerType &val) {
    out.write(reinterpret_cast<const char *>(&val), sizeof(LayerType));
    return out;
}
std::istream & operator >> (std::istream &in, LayerType &val) {
    in.read(reinterpret_cast<char *>(&val), sizeof(LayerType));
    return in;
}
/** @brief 层抽象基类：持有输入 / 输出 / 梯度与序列化接口 */
struct Layer {
    FeatureExtent in_extent{}, out_extent{};
    FeatureMap in{}, out{}, gradient{};
    int stride{};
    LayerType layer_type = LayerType::None;

    Layer() = default;
    Layer(FeatureExtent in_size, FeatureExtent out_size)
        :   in_extent(in_size.x, in_size.y, in_size.z),
            out_extent(out_size.x, out_size.y, out_size.z),
            in(FeatureMap(in_size.z, in_size.x, in_size.y)),
            out(FeatureMap(out_size.z, out_size.x, out_size.y)),
            gradient(FeatureMap(in_size.z, in_size.x, in_size.y)) {  }

    /** @brief 前向传播
     *  @param in 上一层的输出
     */
    virtual void forward(const FeatureMap& in) = 0;
    /** @brief 反向传播，结果写入 gradient
     *  @param next_gradient 来自下一层的梯度
     */
    virtual void backward(const FeatureMap& next_gradient) = 0;
    /** @brief 从二进制流读入层状态 */
    virtual void read(std::istream &in) {
        in >> in_extent >> out_extent;
        in >> this->in >> this->out >> this->gradient;
        in.read(reinterpret_cast<char *>(&stride), sizeof(stride));
        in >> layer_type;
    }
    /** @brief 按与 read 相同的格式写出层状态 */
    virtual void write(std::ostream &out) {
        out << in_extent << out_extent;
        out << this->in << this->out << this->gradient;
        out.write(reinterpret_cast<const char*>(&stride), sizeof(stride));
        out << layer_type;
    }
    virtual ~Layer() = default;
};

/** @brief 卷积层：valid 卷积，卷积核初始化为 [0, 1/N) 随机值（N = 核面积 × 通道数） */
class ConvLayer : public Layer {
    Kernels kernels{}, filters_grads{}, old_filters_grads{};
public:
    ConvLayer() = default;
	/** @brief 构造卷积层
	 *  @param kernel_num 卷积核个数，即输出通道数
	 *  @param kernel_size 卷积核边长
	 *  @param stride 卷积步长
	 *  @param in_size 输入尺寸
	 *  @note 输出宽高按 (in - kernel_size) / stride + 1 计算，不整除时向下取整
	 */
	ConvLayer(int kernel_num, int kernel_size, int stride, FeatureExtent in_size)
		: Layer(in_size, {(in_size.x - kernel_size) / stride + 1, (in_size.y - kernel_size) / stride + 1, kernel_num}) {
	    this->layer_type = LayerType::Conv;
		this->stride = stride;
	    kernels = Kernels(kernel_num, in_size.z, kernel_size, kernel_size);

		int N = kernel_size * kernel_size * in_size.z;
	    kernels.foreach([=](auto &val) {
	        val = 1.0f / N * std::rand() / 2147483647.0;
	    });
	    filters_grads = Kernels(kernel_num, in_size.z, kernel_size, kernel_size);
	    old_filters_grads = Kernels(kernel_num, in_size.z, kernel_size, kernel_size);
	}

	/** @brief 卷积前向：输出[z,x,y] 为输入 patch 与第 z 个卷积核的点积 */
	void forward(const FeatureMap& in) override {
		this->in = in;

	    for (int z = 0; z < out_extent.z; ++z) {
	        for (int x = 0; x < out_extent.x; ++x) {
	            for (int y = 0; y < out_extent.y; ++y) {
	                float sum{};
	                const int h_start = x * stride ;
	                const int w_start = y * stride ;

	                for (int kz = 0; kz < kernels.extent(1); ++kz) {
	                    for (int kh = 0; kh < kernels.extent(2); ++kh) {
	                        for (int kw = 0; kw < kernels.extent(3); ++kw) {
	                            const int ih = h_start + kh;
	                            const int iw = w_start + kw;
	                            sum += in[kz, ih, iw] * kernels[z, kz, kh, kw];
	                        }
	                    }
	                }

	                out[z, x, y] = sum;
	            }
	        }
	    }

	}

    /** @brief 卷积反向：累加卷积核梯度与输入梯度，并立即更新卷积核 */
    void backward(const FeatureMap& next_gradient) override {
	    filters_grads.foreach([](auto &val) { val = 0; });
	    gradient.foreach([](auto &val) { val = 0; });

	    for (int z = 0; z < out_extent.z; ++z) {
	        for (int x = 0; x < out_extent.x; ++x) {
	            for (int y = 0; y < out_extent.y; ++y) {
	                const float grad_val = next_gradient[z, x, y];
	                const int h_start = x * stride;
	                const int w_start = y * stride;

	                for (int kz = 0; kz < kernels.extent(1); ++kz) {
	                    for (int kh = 0; kh < kernels.extent(2); ++kh) {
	                        for (int kw = 0; kw < kernels.extent(3); ++kw) {
	                            const int in_h = h_start + kh;
	                            const int in_w = w_start + kw;
                                filters_grads[z, kz, kh, kw] += in[kz, in_h, in_w] * grad_val;
                                gradient[kz, in_h, in_w] += kernels[z, kz, kh, kw] * grad_val;
	                        }
	                    }
	                }
	            }
	        }
	    }

	    for (int cur_kernel = 0; cur_kernel < kernels.extent(0); cur_kernel++) {
            for (int z = 0; z < kernels.extent(1); z++) {
                for (int x = 0; x < kernels.extent(2); x++) {
                    for (int y = 0; y < kernels.extent(3); y++) {
	                    float& w = kernels[cur_kernel, z, x, y];
	                    auto& grad = filters_grads[cur_kernel, z, x, y];
	                    auto& old_grad = old_filters_grads[cur_kernel, z, x, y];
	                    update_weight(w, grad, old_grad);
	                    update_gradient(grad, old_grad);
	                }
	            }
	        }
	    }
	}

    /** @brief 读入卷积核与梯度缓存 */
    void read(std::istream &in) override {
	    Layer::read(in);
	    in >> kernels >> filters_grads >> old_filters_grads;
	}

    /** @brief 写出卷积核与梯度缓存 */
    void write(std::ostream &out) override {
	    Layer::write(out);
	    out << kernels << filters_grads << old_filters_grads;
	}
};

/** @brief 激活层：逐元素套用激活函数，默认 ReLU */
class ActionLayer: public Layer {

    std::shared_ptr<Activate> action_ = std::make_shared<Relu>();
    ActivateType activate_type = ActivateType::Relu;

public:
    ActionLayer() = default;

    explicit ActionLayer(const FeatureExtent in_size)
        : Layer(in_size, in_size) {
        this->layer_type = LayerType::Activate;
    }

    /** @brief 激活前向 */
    void forward(const Tensor<float, 3>& in) override {
        this->in = in;
        for (int z = 0; z < in.extent(0); z++)
            for (int i = 0; i < in.extent(1); i++)
                for (int j = 0; j < in.extent(2); j++)
                    out[z, i, j] = action_->action(in[z, i, j]);
    }

    /** @brief 激活反向：梯度乘以激活函数在输入处的导数 */
    void backward(const FeatureMap& next_gradient) override {
        for (int z = 0; z < in.extent(0); z++)
            for (int i = 0; i < in.extent(1); i++)
                for (int j = 0; j < in.extent(2); j++)
                    gradient[z, i, j] = action_->deaction(in[z, i, j]) * next_gradient[z, i, j];

    }

    /** @brief 读入激活函数类型并据此重建激活对象 */
    void read(std::istream &in) override {
        Layer::read(in);
        in >> activate_type;
        switch (activate_type) {
        case ActivateType::Sigmoid:
            action_ = std::make_shared<Sigmoid>();
            break;
        case ActivateType::Relu:
            action_ = std::make_shared<Relu>();
            break;
        default:
            throw std::invalid_argument("Invalid activate type");
        }
    }

    /** @brief 写出激活函数类型 */
    void write(std::ostream &out) override {
        Layer::write(out);
        out << activate_type;
    }
};

/** @brief 最大池化层：前向记录最大值坐标，反向只回传该位置 */
export class PoolLayer: public Layer {
    PoolWindow pool_window_;
    PoolLayerMaxPos pool_max_pos_;

    int pool_window_size;
public:

    PoolLayer() = default;

	/** @brief 构造最大池化层
	 *  @param pool_window_size 池化窗口边长
	 *  @param stride 滑动步长
	 *  @param in_size 输入尺寸
	 */
	PoolLayer(int pool_window_size, int stride, FeatureExtent in_size)
		: pool_window_size(pool_window_size),
		Layer(in_size, {(in_size.x - pool_window_size) / stride + 1, (in_size.y - pool_window_size) / stride + 1, in_size.z}) {
        this->layer_type = LayerType::Pool;
	    this->stride = stride;
	    pool_window_ = PoolWindow(pool_window_size, pool_window_size);
	    pool_max_pos_ = PoolLayerMaxPos(in_size.z, (in_size.x - pool_window_size) / stride + 1, (in_size.y - pool_window_size) / stride + 1);
	}

	/** @brief 池化前向：取窗口内最大值，同时记录其坐标 */
	void forward(const FeatureMap& in) override {
	    this->in = in;

	    for (int cur_cannel = 0; cur_cannel < out_extent.z; ++cur_cannel) {
	        for (int oh = 0; oh < out_extent.x; ++oh) {
	            for (int ow = 0; ow < out_extent.y; ++ow) {

	                const int start_x = oh * stride;
	                const int start_y = ow * stride;

                    int max_x{}, max_y{};
	                float cur_max = std::numeric_limits<float>::lowest();
	                for (int ph = 0; ph < pool_window_.extent(0); ++ph) {
	                    for (int pw = 0; pw < pool_window_.extent(1); ++pw) {
	                        const int in_x = start_x + ph;
	                        const int in_y = start_y + pw;

	                        if (cur_max < in[cur_cannel, in_x, in_y]) {
	                            max_x = in_x;
	                            max_y = in_y;
	                            cur_max = in[cur_cannel, in_x, in_y];
	                        }
	                    }
	                }
	                out[cur_cannel, oh, ow] = cur_max;
	                pool_max_pos_[cur_cannel, oh, ow].x = max_x;
	                pool_max_pos_[cur_cannel, oh, ow].y = max_y;

	            }
	        }
	    }
	}

	/** @brief 池化反向：梯度只回传到前向记录的最大值位置 */
	void backward(const FeatureMap& next_gradient) override {
	    gradient.foreach([](float &val) {
            val = 0.f;
        });

	    for (int z = 0; z < out_extent.z; ++z) {
	        for (int x = 0; x < out_extent.x; ++x) {
	            for (int y = 0; y < out_extent.y; ++y) {
	                auto &[max_x, max_y] = pool_max_pos_[z, x, y];
	                gradient[z, max_x, max_y] = next_gradient[z, x, y];
	            }
	        }
	    }
	}

    /** @brief 读入池化窗口与最大值位置缓存 */
    void read(std::istream &in) override {
	    Layer::read(in);
	    in >> pool_window_ >> pool_max_pos_;
	}

    /** @brief 写出池化窗口与最大值位置缓存 */
    void write(std::ostream &out) override {
	    Layer::write(out);
	    out << pool_window_ << pool_max_pos_;
	}
};

/** @brief 全连接层：特征图展平后与权重做内积，默认 Sigmoid 激活 */
export class FCLayer: public Layer {
    std::shared_ptr<Activate> action_ = std::make_shared<Sigmoid>();
    ActivateType activate_type = ActivateType::Sigmoid;
    Weights weights;
public:
	Label fc_in, fc_out;
	Vector<float> once_gradient;
    Vector<float> once_old_gradient;

    FCLayer() = default;
	/** @brief 构造全连接层，权重以 [-1, 1) 随机初始化
	 *  @param in_size 输入尺寸（会被展平成一维）
	 *  @param out_size 输出维度
	 *  @note 权重随机化使用全局随机引擎，需可复现时先调用 seed_random()
	 */
	FCLayer(FeatureExtent in_size, int out_size)
		: Layer(in_size, {out_size, 1, 1}),
        weights(out_size, in_size.z, in_size.x, in_size.y) {
		fc_in = Label(out_size);
	    fc_out = Label(out_size);

	    this->layer_type = LayerType::FC;
		once_gradient = Label(out_size);
	    once_old_gradient = Label(out_size);

	    random_tensor(weights, -1, 1);
	}

	/** @brief 全连接前向：加权和与激活后的输出分别存于 fc_in / fc_out */
	void forward(const FeatureMap& in) override {
	    this->in = in;

	    for (int cur_kernel = 0; cur_kernel < weights.extent(0); ++cur_kernel) {
	        float sum{};
	        for (int z = 0; z < weights.extent(1); ++z) {
	            for (int x = 0; x < weights.extent(2); x++) {
	                for (int y = 0; y < weights.extent(3); y++) {
	                    sum += in[z, x, y] * weights[cur_kernel, z, x, y];
	                }
	            }
	        }
	        fc_in[cur_kernel] = sum;
	        fc_out[cur_kernel] = action_->action(sum);
	    }
	}

	/** @brief 基类的反向接口，未实现；请调用 backward(Vector<float>&) */
	void backward(const FeatureMap& next_gradient) override {  }
	/** @brief 全连接反向：更新权重，并把梯度回传到输入特征图
	 *  @param grad_next_layer 来自上一层的梯度
	 */
	void backward(Vector<float>& grad_next_layer) {
	    gradient.foreach([](auto &val) {
	        val = 0;
	    });

		for (int n = 0; n < out.extent(1); n++) {
			auto& grad = once_gradient[n];
		    auto& old_grad = once_old_gradient[n];

			grad = grad_next_layer[ n ] * action_->deaction(fc_in[n]);

            for (int k = 0; k < in.extent(0); k++) {
                for (int i = 0; i < in.extent(1); i++) {
                    for (int j = 0; j < in.extent(2); j++) {
                        gradient[k, i, j] += grad * weights[n, k, i, j];
                        auto &w = weights[n, k, i, j];
                        update_weight(w, grad, old_grad, in[k, i, j]);
                    }
                }
            }
		    update_gradient(grad, old_grad);
		}
	}

    /** @brief 读入激活函数类型、权重与梯度缓存 */
    void read(std::istream &in) override {
	    Layer::read(in);
	    in >> activate_type;
	    switch (activate_type) {
        case ActivateType::Sigmoid:
	        action_ = std::make_shared<Sigmoid>();
	        break;
        case ActivateType::Relu:
	        action_ = std::make_shared<Relu>();
	        break;
        default:
	        throw std::invalid_argument("Invalid activate type");
	    }
	    in >> weights >> fc_in >> fc_out >> once_gradient >> once_old_gradient;
	}


    /** @brief 写出激活函数类型、权重与梯度缓存 */
    void write(std::ostream &out) override {
	    Layer::write(out);
	    out << activate_type;
	    out << weights << fc_in << fc_out << once_gradient << once_old_gradient;
	}

};

/** @brief 卷积神经网络：按添加顺序组织各层，支持训练 / 推理 / 序列化 */
export class CNN {
    int version_ = 1;
    FeatureExtent in_extent;
    std::vector<std::shared_ptr<Layer>> layers;

    [[nodiscard]] FeatureExtent get_cur_in_extent() const {
        if (layers.empty())
            return in_extent;
        return layers.back()->out_extent;
    }

    auto get_out() const {
        return dynamic_cast<FCLayer*>(layers.back().get())->fc_out;
    }

public:
    CNN() = default;
	/** @brief 构造网络并指定输入尺寸
	 *  @param x 输入宽
	 *  @param y 输入高
	 *  @param z 输入通道数
	 */
	CNN(int x, int y, int z) : in_extent(x, y, z) {  }

    /** @brief 数据集加载回调，供按路径训练 / 测试的重载使用；未设置时调用会抛异常 */
    std::function<std::vector<Data>(const std::string&, const std::string&)> load_dataset_func;

	/** @brief 追加卷积层，输入尺寸取自上一层的输出 */
	void add_conv(int kernel_num, int kernel_size, int stride = 1) {
		layers.emplace_back(std::make_shared<ConvLayer>(kernel_num, kernel_size, stride, get_cur_in_extent()));
	}

	/** @brief 追加激活层（ReLU） */
	void add_relu() {
	    auto cur_in_extent = get_cur_in_extent();
	    auto layer = std::make_shared<ActionLayer>(cur_in_extent);
		layers.push_back(layer);
	}

	/** @brief 追加最大池化层 */
	void add_pool(int pool_window_size, int stride) {
		layers.emplace_back(std::make_shared<PoolLayer>(pool_window_size, stride, get_cur_in_extent()));
	}

	/** @brief 追加全连接层，输出维度为类别数 */
	void add_fc(int type_count) {
		layers.emplace_back(std::make_shared<FCLayer>(get_cur_in_extent(), type_count));
	}

	/** @brief 前向传播：逐层传递，结果存于各层的 out */
	void forward(const FeatureMap& data) const {
	    auto begin_layer = layers.front();
	    begin_layer->forward(data);
		for (int i = 1; i < layers.size(); i++)
			layers[i]->forward(layers[i - 1]->out);
	}
    /** @brief 反向传播：从输出层误差起算，逐层回传
     *  @param res 期望的标签向量
     */
    void backward(const Label& res) const {
	    auto end_label = dynamic_cast<FCLayer *>(layers.back().get());
	    auto vec_res_info = end_label->fc_out - res;
	    end_label->backward(vec_res_info);
	    for (int i = layers.size() - 2; i >= 0; i--)
	        layers[i]->backward(layers[i + 1]->gradient);
	}

    /** @brief 推理：前向传播并返回输出层结果 */
    Label forecast(const FeatureMap& data) const {
	    forward(data);
	    return get_out();
	}
    /** @brief 单样本训练：一次前向加一次反向 */
    void train(const FeatureMap &data, const Label& label) const {
		forward(data);
	    backward(label);
	}
    /** @brief 批量训练，每轮遍历整个数据集
     *  @param datas 训练集
     *  @param train_count 训练轮数
     *  @note 打印间隔为 datas.size() / 100，样本数不足 100 时该值为 0，取模会除零
     */
    void train(const std::vector<Data> &datas,int train_count) const {
	    // Progress progress(false);
	    // for (int i = 0;i < train_count; ++i)
	        // progress.push("训练中...", datas.size());

	    int flag = datas.size() / 100;
	    for (int i = 0;i < train_count; ++i) {
	        float acc{};
	        std::thread print_thread;
	        for (int cur = 0; cur < datas.size(); ++cur) {
	            auto &[image, label] = datas[cur];
	            // progress.cur_bar() += 1;

	            if (cur % flag == 0) {
                    // progress.set_info("acc is : " + std::to_string(acc / cur));
	                print_thread = std::thread([&] {
                        // progress.print();
                    });
	            }

	            train(image, label);
	            auto res_type = OneHot::out_to_type(label);
	            auto out_type = OneHot::out_to_type(get_out());
	            acc += res_type == out_type;

	            if (cur % flag == 0)
	                print_thread.join();
	        }
	        // progress.print();
	        // progress += 1;
	    }

	}
    /** @brief 按路径加载数据集后训练，需先设置 load_dataset_func
     *  @throw std::runtime_error load_dataset_func 未设置
     */
    void train(const std::string &image_path, const std::string &label_path, int train_count) const {
	    if (!load_dataset_func)
	        throw std::runtime_error("load_dataset_func impl is null");
	    auto dataset = load_dataset_func(image_path, label_path);
	    train(dataset, train_count);
	}
    /** @brief 单样本测试：预测类别与标签一致时返回 true */
    bool test(const FeatureMap &data, const Label& label) const {
	    auto out = OneHot::out_to_type(forecast(data));
	    auto lab = OneHot::out_to_type(label);
	    return out == lab;
	}
    /** @brief 批量测试，打印准确率 */
    void test(const std::vector<Data> &datas) const {
	    // Progressbar pb("测试中...", datas.size());

	    int acc{};
	    for (int i = 0;i < datas.size(); ++i) {
	        auto [image, label] = datas[i];
	        acc += test(image, label);
	        // pb += 1;
	        // pb.print();
	    }
	    endl(std::cout);
	    std::cout << "acc is : " << acc * 1.f / datas.size() << std::endl;
	}
    /** @brief 按路径加载数据集后测试，需先设置 load_dataset_func
     *  @throw std::runtime_error load_dataset_func 未设置
     */
    void test(const std::string &image_path, const std::string &label_path) const {
	    if (!load_dataset_func)
	        throw std::runtime_error("load_dataset_func impl is null");
	    auto dataset = load_dataset_func(image_path, label_path);
	    test(dataset);
	}

    /** @brief 从二进制文件恢复网络结构与权重
     *  @param module_path 模型文件路径
     *  @throw std::runtime_error 文件打不开或层类型不支持
     *  @note 读取到的层是追加进 layers 的，不会覆盖已有的层
     */
    void load(const std::string &module_path) {
	    std::ifstream file(module_path, std::ios::binary);
	    if (!file.is_open())
	        throw std::runtime_error("load " + module_path + " failed");

        // 序列化版本信息
	    file.read(reinterpret_cast<char *>(&version_), sizeof(version_));

	    // 输入尺寸信息
	    in_extent.read(file);

	    // 层数
	    int layers_count = layers.size();
	    file.read(reinterpret_cast<char *>(&layers_count), sizeof(layers_count));
	    for (int i = 0;i < layers_count; ++i) {
	        LayerType layer_type;
	        std::shared_ptr<Layer> layer;
	        file >> layer_type;
	        // file.read(reinterpret_cast<char *>(&layer_type), sizeof(layer_type));
	        switch (layer_type) {
	        case LayerType::Conv:
	            layer = std::make_shared<ConvLayer>();
                break;
	        case LayerType::Activate:
	            layer = std::make_shared<ActionLayer>();
	            break;
            case LayerType::Pool:
	            layer = std::make_shared<PoolLayer>();
                break;
            case LayerType::FC:
	            layer = std::make_shared<FCLayer>();
                break;
            default:
	            throw std::runtime_error("layer type not support");
	        }
	        layers.emplace_back(std::move(layer));
	    }

	    for (auto &layer : layers)
	        layer->read(file);

	}
    /** @brief 把网络结构与权重写入二进制文件
     *  @param module_path 模型文件路径
     *  @throw std::runtime_error 文件无法创建
     */
    void save(const std::string &module_path) {
	    std::ofstream file(module_path, std::ios::binary);
	    if (!file.is_open())
	        throw std::runtime_error("save " + module_path + " failed");

	    // 序列化版本
	    file.write(reinterpret_cast<char *>(&version_), sizeof(version_));

	    // 输入尺寸信息
	    in_extent.write(file);

	    // 层数
	    int layers_count = layers.size();
	    file.write(reinterpret_cast<char *>(&layers_count), sizeof(layers_count));

	    // 层类型
	    for (const auto &layer : layers) {
	        file.write(reinterpret_cast<char *>(&layer->layer_type), sizeof(layer->layer_type));
	    }
	    for (auto &layer : layers)
	        layer->write(file);
	}
};

/** @brief 一次性把整个文件读入 new[] 出来的缓冲区
 *  @return 缓冲区首地址，调用方负责 delete[]；空文件时返回 nullptr
 *  @throw std::runtime_error 文件不存在
 */
char* read_file(const std::string &path) {
    if (!std::filesystem::exists(path))
        throw std::runtime_error("read_file " + path + " failed");

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	if (size == -1)
		return nullptr;

    auto buffer = new char[size];

	file.read(reinterpret_cast<char *>(buffer), size);
	return buffer;
}
/** @brief 加载 MNIST 原始文件（idx 格式）：图像归一化到 [0, 1]，标签转 One-Hot
 *  @param image_path 图像文件路径
 *  @param label_path 标签文件路径
 *  @return 数据集；图像固定 1×28×28，标签为长度 10 的 One-Hot
 *  @note 文件头按大端解析（std::byteswap），样本数取自图像文件偏移 4 处
 */
export std::vector<Data> load_mnist_dataset(const std::string& image_path, const std::string& label_path) {
	std::vector<Data> ret;

	char* train_image = read_file(image_path);
	char* train_labels = read_file(label_path);

	int case_count = std::byteswap(*reinterpret_cast<int *>(train_image + 4));

	for (int i = 0; i < case_count; i++) {
		Data c {
			Tensor<float, 3>(1, 28, 28),
			Vector<float>(10)
		};

		char* img = train_image + 16 + i * (28 * 28);
		char* label = train_labels + 8 + i;

		for (int x = 0; x < 28; x++)
			for (int y = 0; y < 28; y++)
				c.data[0, x, y] = img[x + y * 28] / 255.f;

		for (int b = 0; b < 10; b++)
			c.label[ b] = *label == b ? 1.0f : 0.0f;

		ret.push_back(c);
	}
	delete[] train_image;
	delete[] train_labels;

	return ret;
}

NAMESPACE_END

#endif