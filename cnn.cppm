/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <cstdint>
export module modforge.deep_learning.cnn;

import modforge.tensor;
import modforge.deep_learning.tools;

using PoolWindow = Tensor<float, 2>;
using FeatureMap = Tensor<float, 3>;
using Kernels    = Tensor<float, 4>;
using Weights    = Tensor<float, 4>;
using Label      = Vector<float>;

export struct FeatureExtent {
    int x{}, y{}, z{};
};

export struct Data {
    Tensor<float, 3> data;
    Label label;
};


#define LEARNING_RATE 0.01
#define MOMENTUM 0.6
#define WEIGHT_DECAY 0.001

void update_weight(float& w, float gradient, float old_gradient, float multp = 1) {
    w -= LEARNING_RATE * (gradient + old_gradient * MOMENTUM) * multp + LEARNING_RATE * WEIGHT_DECAY * w;
}

void update_gradient(float &gradient, float &old_gradient) {
    old_gradient = (gradient + old_gradient) * MOMENTUM;
}


export struct Layer {
    FeatureExtent in_extent, out_extent;

    FeatureMap in, out, gradient;
    Layer( FeatureExtent in_size, FeatureExtent out_size )
        : in_extent(in_size.x, in_size.y, in_size.z), out_extent(out_size.x, out_size.y, out_size.z) {
        
        in = FeatureMap(in_size.z, in_size.x, in_size.y);
        gradient = FeatureMap(in_size.z, in_size.x, in_size.y);
        out = FeatureMap(out_size.z, out_size.x, out_size.y);

    }

    virtual void forward(const FeatureMap& in )=0;
    virtual void backward(const FeatureMap& grad_next_layer )=0;
    virtual ~Layer() = default;
};

export class ConvLayer : public Layer {
public:
    Kernels kernels;
    Tensor<float, 4> filters_grads, old_filters_grads;

	uint16_t stride;
	uint16_t extend_filter;

	// 卷积层构造仅初始化卷积核
	ConvLayer( uint16_t stride, uint16_t extend_filter, uint16_t number_filters, FeatureExtent in_size )
		: Layer(in_size, {(in_size.x - extend_filter) / stride + 1, (in_size.y - extend_filter) / stride + 1, number_filters} ) {
		this->stride = stride;
		this->extend_filter = extend_filter;
	    kernels = Kernels(number_filters, in_size.z, extend_filter, extend_filter);

		int N = extend_filter * extend_filter * in_size.z;
	    kernels.foreach([=](auto &val) {
	        val = 1.0f / N * rand() / 2147483647.0;
	    });
	    filters_grads = Kernels(number_filters, in_size.z, extend_filter, extend_filter);
	    old_filters_grads = Kernels(number_filters, in_size.z, extend_filter, extend_filter);
	}

	struct range_t {
		int min_x, min_y, min_z;
		int max_x, max_y, max_z;
	};

	int GET_R( float f, int max, bool lim_min ) {
		if ( f <= 0 ) return 0;
		max -= 1;
		if ( f >= max ) return max;
		if ( lim_min ) return ceil( f );
		else return floor( f );
	}

	range_t map_to_output( int x, int y ) {
		float a = x, b = y;
		return {
		    GET_R( (a - extend_filter + 1) / stride, out.extent(1), true ),
		    GET_R( (b - extend_filter + 1) / stride, out.extent(2), true ),
		    0,
		   GET_R( a / stride, out.extent(1), false ),
		   GET_R( b / stride, out.extent(2), false ),
		   (int)out.extent(0) - 1, };
	}

	void forward(const FeatureMap& in ) override {
		this->in = in;

	    int kernel_z = kernels.extent(1);
	    int kernel_h = kernels.extent(2);
	    int kernel_w = kernels.extent(3);

	    for (int z = 0; z < out_extent.z; ++z) {

	        for (int x = 0; x < out_extent.x; ++x) {
	            for (int y = 0; y < out_extent.y; ++y) {
	                float sum{};

	                const int h_start = x * stride ;
	                const int w_start = y * stride ;

	                for (int kz = 0; kz < kernel_z; ++kz) {
	                    for (int kh = 0; kh < kernel_h; ++kh) {
	                        for (int kw = 0; kw < kernel_w; ++kw) {

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
	    // 清空卷积核梯度
	    filters_grads.foreach([](auto &val) { val = 0; });

	    // 清空输入梯度（注意：gradient张量在Layer构造时已初始化为输入尺寸）
	    gradient.foreach([](auto &val) { val = 0; });

	    const int kernel_z = kernels.extent(1); // 输入通道数
	    const int kernel_h = kernels.extent(2); // 卷积核高度
	    const int kernel_w = kernels.extent(3); // 卷积核宽度

	    // 遍历输出梯度的每个位置
	    for (int z = 0; z < out_extent.z; ++z) {        // 输出通道
	        for (int x = 0; x < out_extent.x; ++x) {    // 输出高度
	            for (int y = 0; y < out_extent.y; ++y) { // 输出宽度
	                const float grad_val = next_gradient[z, x, y];
	                const int h_start = x * stride;
	                const int w_start = y * stride;

	                // 遍历卷积核覆盖的输入区域
	                for (int kz = 0; kz < kernel_z; ++kz) {       // 输入通道
	                    for (int kh = 0; kh < kernel_h; ++kh) {   // 核高度偏移
	                        for (int kw = 0; kw < kernel_w; ++kw) { // 核宽度偏移
	                            const int in_h = h_start + kh;
	                            const int in_w = w_start + kw;

	                            // 边界检查
	                            if (in_h >= 0 && in_h < in_extent.x &&
                                    in_w >= 0 && in_w < in_extent.y) {
	                                // 更新卷积核梯度：输入值 * 输出梯度
	                                filters_grads[z, kz, kh, kw] +=
                                        in[kz, in_h, in_w] * grad_val;

	                                // 更新输入梯度：卷积核权重 * 输出梯度
	                                gradient[kz, in_h, in_w] +=
                                        kernels[z, kz, kh, kw] * grad_val;
                                    }
	                        }
	                    }
	                }
	            }
	        }
	    }

	    for ( int a = 0; a < kernels.extent(0); a++ )
	        for ( int i = 0; i < extend_filter; i++ )
	            for ( int j = 0; j < extend_filter; j++ )
	                for ( int z = 0; z < in.extent(0); z++ )
	                {
	                    float& w = kernels[a, z, i, j];
	                    auto& grad = filters_grads[a, z, i, j];
	                    auto& old_grad = old_filters_grads[a, z, i, j];
	                    update_weight( w, grad, old_grad );
	                    update_gradient( grad, old_grad );
	                }

	}
};


// 非线性激励层
export class ActionLayer: public Layer {

    std::shared_ptr<Activation> action_ = std::make_shared<Relu>();

public:

    ActionLayer( FeatureExtent in_size )
        : Layer( in_size, in_size ) {  }

    void forward(const Tensor<float, 3>& in ) override {
        this->in = in;

        for ( int i = 0; i < in.extent(1); i++ )
            for ( int j = 0; j < in.extent(2); j++ )
                for ( int z = 0; z < in.extent(0); z++ )
                    out[z, i, j] = action_->action(in[z, i, j]);

    }


    void backward(const FeatureMap& next_gradient ) override {

        for ( int z = 0; z < in.extent(0); z++ )
            for ( int i = 0; i < in.extent(1); i++ )
                for ( int j = 0; j < in.extent(2); j++ )
                    gradient[z, i, j] = action_->deaction(in[z, i, j]) * next_gradient[z, i, j];

    }
};


export class PoolLayer: public Layer {
    PoolWindow pool_window_;
public:
	uint16_t stride;
	uint16_t extend_filter;

	PoolLayer( uint16_t stride_, uint16_t extend_filter_, FeatureExtent in_size )
		: stride(stride_), extend_filter(extend_filter_),
		Layer( in_size, {(in_size.x - extend_filter_) / stride_ + 1, (in_size.y - extend_filter_) / stride_ + 1, in_size.z} ) {

	    pool_window_ = PoolWindow(extend_filter, extend_filter);
	}

	FeatureExtent map_to_input( FeatureExtent out_size, int z ) {
		return {out_size.x * stride, out_size.y * stride, z};
	}

	void forward(const FeatureMap& in ) override {
	    this->in = in;

	    const int pool_h = pool_window_.extent(0);
	    const int pool_w = pool_window_.extent(1);


	    for (int cur_cannel = 0; cur_cannel < out_extent.z; ++cur_cannel) {
	        for (int oh = 0; oh < out_extent.x; ++oh) {
	            for (int ow = 0; ow < out_extent.y; ++ow) {

	                float cur_max = std::numeric_limits<float>::lowest();

	                const int start_x = oh * stride;
	                const int start_y = ow * stride;

	                for (int ph = 0; ph < pool_h; ++ph) {
	                    for (int pw = 0; pw < pool_w; ++pw) {
	                        const int in_x = start_x + ph;
	                        const int in_y = start_y + pw;

	                        if (cur_max < in[cur_cannel, in_x, in_y])
	                            cur_max = in[cur_cannel, in_x, in_y];
	                    }
	                }

	                out[cur_cannel, oh, ow] = cur_max;
	            }
	        }
	    }
	}


	void backward(const FeatureMap& next_gradient ) override {
	    gradient.foreach([](float &val) {
            val = 0.f;
        });

	    for (int c = 0; c < out_extent.z; ++c) {
	        for (int oh = 0; oh < out_extent.x; ++oh) {
	            for (int ow = 0; ow < out_extent.y; ++ow) {
	                int max_h = -1, max_w = -1;
	                float max_val = std::numeric_limits<float>::lowest();
	                // 遍历池化窗口
	                for (int ph = 0; ph < pool_window_.extent(0); ++ph) {
	                    for (int pw = 0; pw < pool_window_.extent(1); ++pw) {
	                        int ih = oh * stride + ph;
	                        int iw = ow * stride + pw;
	                        // if (ih < 0 || ih >= in_extent.h || iw < 0 || iw >= in_extent.w)
	                        //     continue;
	                        if (in[c, ih, iw] > max_val) {
	                            max_val = in[c, ih, iw];
	                            max_h = ih;
	                            max_w = iw;
	                        }
	                    }
	                }

	                // 只在最大值位置回传梯度
	                if (max_h != -1 && max_w != -1) {
	                    gradient[c, max_h, max_w] = next_gradient[c, oh, ow];
	                }
	            }
	        }
	    }
	    this->gradient = std::move(gradient);
	}
};
export class FCLayer: public Layer {
    std::shared_ptr<Activation> action_ = std::make_shared<Sigmoid>();
public:
	Label fc_in, fc_out;
    Weights weights;
	Vector<float> once_gradient;
    Vector<float> once_old_gradient;

	FCLayer( FeatureExtent in_size, int out_size )
		: Layer( in_size, {out_size, 1, 1} ),
        weights(out_size, in_size.z, in_size.x, in_size.y) {
		fc_in = Label( out_size );
	    fc_out = Label( out_size );

		once_gradient = Label( out_size );
	    once_old_gradient = Label( out_size );

	    random_tensor(weights, -1, 1);
	}



	void forward(const FeatureMap& in ) override {
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


	void backward(const FeatureMap& next_gradient ) override {  }
	void backward( Vector<float>& grad_next_layer ) {
	    gradient.foreach([](auto &val) {
	        val = 0;
	    });

		for ( int n = 0; n < out.extent(1); n++ ) {
			auto& grad = once_gradient[n];
		    auto& old_grad = once_old_gradient[n];

			grad = grad_next_layer[ n ] * action_->deaction( fc_in[n] );

            for ( int k = 0; k < in.extent(0); k++ ) {
                for ( int i = 0; i < in.extent(1); i++ ) {
                    for ( int j = 0; j < in.extent(2); j++ ) {
                        gradient[k, i, j] += grad * weights[n, k, i, j];

                        auto &w = weights[n, k, i, j];
                        update_weight( w, grad, old_grad, in[k, i, j] );
                    }
                }
            }
		    update_gradient( grad, old_grad );
		}
	}

};

export class CNN {
    FeatureExtent in_extent;

    [[nodiscard]] FeatureExtent get_cur_in_extent() const {
        if (layers.empty())
            return in_extent;
        return layers.back()->out_extent;
    }

    std::vector<std::shared_ptr<Layer>> layers;
public:
	CNN(int x, int y, int z) : in_extent( x, y, z ) {  }

	void add_conv( uint16_t stride, uint16_t extend_filter, uint16_t number_filters) {
		layers.push_back( std::make_shared<ConvLayer>(stride, extend_filter, number_filters, get_cur_in_extent()));
	}

	void add_relu() {
		layers.push_back(std::make_shared<ActionLayer>(get_cur_in_extent()));
	}

	void add_pool( uint16_t stride, uint16_t extend_filter) {
		layers.push_back(std::make_shared<PoolLayer>(stride, extend_filter, get_cur_in_extent()));
	}

	void add_fc( int type_count ) {
		layers.push_back(std::make_shared<FCLayer>(get_cur_in_extent(), type_count));
	}

	int predict() {
	    auto end_layer = dynamic_cast<FCLayer*>( layers.back().get() );
	    return OneHot::out_to_type(end_layer->fc_out);
	}

	Tensor<float, 3>& predict_info() {
		return layers.back()->out;
	}

	void forward(const Tensor<float, 3>& data ) {
	    auto begin_layer = layers.front();
	    begin_layer->forward(data);
		for ( int i = 1; i < layers.size(); i++ )
			layers[i]->forward(layers[i - 1]->out);
	}
    void backward(const Vector<float> &res) {

	    auto end_label = dynamic_cast<FCLayer *>(layers.back().get());
	    auto vec_res_info = end_label->fc_out - res;
	    end_label->backward( vec_res_info );
	    for ( int i = layers.size() - 2; i >= 0; i-- )
	        layers[i]->backward( layers[i + 1]->gradient);

	}


	float train(const Tensor<float, 3> &data, const Vector<float>& label)	{

		// 前向传播
		forward( data );
	    backward(label);

		// // 最后一层，目前为全连接层
		// // 和最后的结果相减得到差值
	    auto end_label = dynamic_cast<FCLayer *>(layers.back().get());
	    auto vec_res_info = end_label->fc_out - label;

	    // 计算误差
		float err = 0;
		for ( int i = 0; i < 10; i++ ) {
			float x = label[i] - vec_res_info[i];
			err += x*x ;
		}
		return sqrt(err) * 100;
	}


};



uint32_t byteswap_uint32(uint32_t a) {
	return ((((a >> 24) & 0xff) << 0) |
		(((a >> 16) & 0xff) << 8) |
		(((a >> 8) & 0xff) << 16) |
		(((a >> 0) & 0xff) << 24));
}


export uint8_t* read_file( const char* szFile ) {
	std::ifstream file( szFile, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios::beg );

	if ( size == -1 )
		return nullptr;

	uint8_t* buffer = new uint8_t[size];
	file.read( (char*)buffer, size );
	return buffer;
}

export std::vector<Data> read_train_cases() {
	std::vector<Data> ret;

	uint8_t* train_image = read_file( "train-images.idx3-ubyte" );
	uint8_t* train_labels = read_file( "train-labels.idx1-ubyte" );

	uint32_t case_count = byteswap_uint32( *(uint32_t*)(train_image + 4) );

	for ( int i = 0; i < case_count; i++ ) {
		Data c {
			Tensor<float, 3>(1, 28, 28),
			Vector<float>( 10 )
		};

		uint8_t* img = train_image + 16 + i * (28 * 28);
		uint8_t* label = train_labels + 8 + i;

		for ( int x = 0; x < 28; x++ )
			for ( int y = 0; y < 28; y++ )
				c.data[0, x, y] = img[x + y * 28] / 255.f;

		for ( int b = 0; b < 10; b++ )
			c.label[ b] = *label == b ? 1.0f : 0.0f;

		ret.push_back( c );
	}
	delete[] train_image;
	delete[] train_labels;

	return ret;
}

export std::vector<Data> read_test_cases() {
	std::vector<Data> cases;

	uint8_t* train_image = read_file( "t10k-images.idx3-ubyte" );
	uint8_t* train_labels = read_file( "t10k-labels.idx1-ubyte" );

	uint32_t case_count = byteswap_uint32( *(uint32_t*)(train_image + 4) );

	for ( int i = 0; i < case_count; i++ ) {
		Data c {
		    Tensor<float, 3>(1, 28, 28),
		    Vector<float> ( 10 )
		};

		uint8_t* img = train_image + 16 + i * (28 * 28);
		uint8_t* label = train_labels + 8 + i;

		for ( int x = 0; x < 28; x++ )
			for ( int y = 0; y < 28; y++ )
				c.data[0, x, y] = img[x + y * 28] / 255.f;

		for ( int b = 0; b < 10; b++ )
			c.label[ b ] = *label == b ? 1.0f : 0.0f;

		cases.push_back( c );
	}
	delete[] train_image;
	delete[] train_labels;

	return cases;
}
