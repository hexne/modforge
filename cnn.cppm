/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
export module modforge.deep_learning.cnn;

import std;
import modforge.tensor;
import modforge.progress;
import modforge.console;
import modforge.deep_learning.tools;

using Label      = Vector<float>;
using Kernels    = Tensor<float, 4>;
using Weights    = Tensor<float, 4>;
using FeatureMap = Tensor<float, 3>;
using PoolWindow = Tensor<float, 2>;

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

export struct Data {
    FeatureMap data;
    Label label;
};

export struct PoolMaxPos {
    int x;
    int y;
};
using PoolLayerMaxPos = Tensor<PoolMaxPos, 3>;


#define LEARNING_RATE 0.01
#define MOMENTUM 0.6
#define WEIGHT_DECAY 0.001

void update_weight(float& w, float gradient, float old_gradient, float multp = 1) {
    w -= LEARNING_RATE * (gradient + old_gradient * MOMENTUM) * multp + LEARNING_RATE * WEIGHT_DECAY * w;
}

void update_gradient(float &gradient, float &old_gradient) {
    old_gradient = (gradient + old_gradient) * MOMENTUM;
}

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

    virtual void forward(const FeatureMap& in) = 0;
    virtual void backward(const FeatureMap& next_gradient) = 0;
    virtual void read(std::istream &in) {
        in >> in_extent >> out_extent;
        in >> this->in >> this->out >> this->gradient;
        in.read(reinterpret_cast<char *>(&stride), sizeof(stride));
        in >> layer_type;
    }
    virtual void write(std::ostream &out) {
        out << in_extent << out_extent;
        out << this->in << this->out << this->gradient;
        out.write(reinterpret_cast<const char*>(&stride), sizeof(stride));
        out << layer_type;
    }
    virtual ~Layer() = default;
};

class ConvLayer : public Layer {
    Kernels kernels{}, filters_grads{}, old_filters_grads{};
public:
    ConvLayer() = default;
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

    void read(std::istream &in) override {
	    Layer::read(in);
	    in >> kernels >> filters_grads >> old_filters_grads;
	}

    void write(std::ostream &out) override {
	    Layer::write(out);
	    out << kernels << filters_grads << old_filters_grads;
	}
};

class ActionLayer: public Layer {

    std::shared_ptr<Activate> action_ = std::make_shared<Relu>();
    ActivateType activate_type = ActivateType::Relu;

public:
    ActionLayer() = default;

    explicit ActionLayer(const FeatureExtent in_size)
        : Layer(in_size, in_size) {
        this->layer_type = LayerType::Activate;
    }

    void forward(const Tensor<float, 3>& in) override {
        this->in = in;
        for (int z = 0; z < in.extent(0); z++)
            for (int i = 0; i < in.extent(1); i++)
                for (int j = 0; j < in.extent(2); j++)
                    out[z, i, j] = action_->action(in[z, i, j]);
    }

    void backward(const FeatureMap& next_gradient) override {
        for (int z = 0; z < in.extent(0); z++)
            for (int i = 0; i < in.extent(1); i++)
                for (int j = 0; j < in.extent(2); j++)
                    gradient[z, i, j] = action_->deaction(in[z, i, j]) * next_gradient[z, i, j];

    }

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

    void write(std::ostream &out) override {
        Layer::write(out);
        out << activate_type;
    }
};

export class PoolLayer: public Layer {
    PoolWindow pool_window_;
    PoolLayerMaxPos pool_max_pos_;

    int pool_window_size;
public:

    PoolLayer() = default;

	PoolLayer(int pool_window_size, int stride, FeatureExtent in_size)
		: pool_window_size(pool_window_size),
		Layer(in_size, {(in_size.x - pool_window_size) / stride + 1, (in_size.y - pool_window_size) / stride + 1, in_size.z}) {
        this->layer_type = LayerType::Pool;
	    this->stride = stride;
	    pool_window_ = PoolWindow(pool_window_size, pool_window_size);
	    pool_max_pos_ = PoolLayerMaxPos(in_size.z, (in_size.x - pool_window_size) / stride + 1, (in_size.y - pool_window_size) / stride + 1);
	}

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

    void read(std::istream &in) override {
	    Layer::read(in);
	    in >> pool_window_ >> pool_max_pos_;
	}

    void write(std::ostream &out) override {
	    Layer::write(out);
	    out << pool_window_ << pool_max_pos_;
	}
};

export class FCLayer: public Layer {
    std::shared_ptr<Activate> action_ = std::make_shared<Sigmoid>();
    ActivateType activate_type = ActivateType::Sigmoid;
    Weights weights;
public:
	Label fc_in, fc_out;
	Vector<float> once_gradient;
    Vector<float> once_old_gradient;

    FCLayer() = default;
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

	void backward(const FeatureMap& next_gradient) override {  }
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

    void read(std::istream &in) override {
	    Layer::read(in);
	    in >> activate_type;
	    switch (activate_type) {
        case ActivateType::Sigmoid:
	        action_ = std::make_shared<Sigmoid>();
        case ActivateType::Relu:
	        action_ = std::make_shared<Relu>();
	        break;
        default:
	        throw std::invalid_argument("Invalid activate type");
	    }
	    in >> weights >> fc_in >> fc_out >> once_gradient >> once_old_gradient;
	}


    void write(std::ostream &out) override {
	    Layer::write(out);
	    out << activate_type;
	    out << weights << fc_in << fc_out << once_gradient << once_old_gradient;
	}

};

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
	CNN(int x, int y, int z) : in_extent(x, y, z) {  }

    std::function<std::vector<Data>(const std::string&, const std::string&)> load_dataset_func;

	void add_conv(int kernel_num, int kernel_size, int stride = 1) {
		layers.emplace_back(std::make_shared<ConvLayer>(kernel_num, kernel_size, stride, get_cur_in_extent()));
	}

	void add_relu() {
	    auto cur_in_extent = get_cur_in_extent();
	    auto layer = std::make_shared<ActionLayer>(cur_in_extent);
		layers.push_back(layer);
	}

	void add_pool(int pool_window_size, int stride) {
		layers.emplace_back(std::make_shared<PoolLayer>(stride, pool_window_size, get_cur_in_extent()));
	}

	void add_fc(int type_count) {
		layers.emplace_back(std::make_shared<FCLayer>(get_cur_in_extent(), type_count));
	}

	void forward(const FeatureMap& data) const {
	    auto begin_layer = layers.front();
	    begin_layer->forward(data);
		for (int i = 1; i < layers.size(); i++)
			layers[i]->forward(layers[i - 1]->out);
	}
    void backward(const Label& res) const {
	    auto end_label = dynamic_cast<FCLayer *>(layers.back().get());
	    auto vec_res_info = end_label->fc_out - res;
	    end_label->backward(vec_res_info);
	    for (int i = layers.size() - 2; i >= 0; i--)
	        layers[i]->backward(layers[i + 1]->gradient);
	}

    Label forcast(const FeatureMap& data) const {
	    forward(data);
	    return get_out();
	}
    void train(const FeatureMap &data, const Label& label) const {
		forward(data);
	    backward(label);
	}
    void train(const std::vector<Data> &datas,int train_count) const {
	    Progress progress(false);
	    for (int i = 0;i < train_count; ++i)
	        progress.push("训练中...", datas.size());

	    int flag = datas.size() / 100;
	    for (int i = 0;i < train_count; ++i) {
	        float acc{};
	        std::thread print_thread;
	        for (int cur = 0; cur < datas.size(); ++cur) {
	            auto &[image, label] = datas[cur];
	            progress.cur_bar() += 1;

	            if (cur % flag == 0) {
                    progress.set_info("acc is : " + std::to_string(acc / cur));
	                print_thread = std::thread([&] {
                        progress.print();
                    });
	            }

	            train(image, label);
	            auto res_type = OneHot::out_to_type(label);
	            auto out_type = OneHot::out_to_type(get_out());
	            acc += res_type == out_type;

	            if (cur % flag == 0)
	                print_thread.join();
	        }
	        progress.print();
	        progress += 1;
	    }

	}
    void train(const std::string &image_path, const std::string &label_path, int train_count) const {
	    if (!load_dataset_func)
	        throw std::runtime_error("load_dataset_func impl is null");
	    auto dataset = load_dataset_func(image_path, label_path);
	    train(dataset, train_count);
	}
    bool test(const FeatureMap &data, const Label& label) const {
	    auto out = OneHot::out_to_type(forcast(data));
	    auto lab = OneHot::out_to_type(label);
	    return out == lab;
	}
    void test(const std::vector<Data> &datas) const {
	    Progressbar pb("测试中...", datas.size());

	    int acc{};
	    for (int i = 0;i < datas.size(); ++i) {
	        auto [image, label] = datas[i];
	        acc += test(image, label);
	        pb += 1;
	        pb.print();
	    }
	    endl(std::cout);
	    std::cout << "acc is : " << acc * 1.f / datas.size() << std::endl;
	}
    void test(const std::string &image_path, const std::string &label_path) const {
	    if (!load_dataset_func)
	        throw std::runtime_error("load_dataset_func impl is null");
	    auto dataset = load_dataset_func(image_path, label_path);
	    test(dataset);
	}

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