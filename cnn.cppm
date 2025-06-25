/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <fstream>
#include <cfloat>
#include <vector>
#include <cmath>
#include <memory>
#include <cstdint>
export module modforge.deep_learning.cnn;

import modforge.tensor;
import modforge.deep_learning.tools;

export struct gradient_t
{
    float grad,oldgrad;
    gradient_t(): grad(0), oldgrad(0) {}
};

#define LEARNING_RATE 0.01
#define MOMENTUM 0.6
#define WEIGHT_DECAY 0.001

export float update_weight( float w, gradient_t& grad, float multp = 1 )
{
    // w -= LEARNING_RATE * grad.grad;
    w -= LEARNING_RATE  * (grad.grad + grad.oldgrad * MOMENTUM) * multp + LEARNING_RATE * WEIGHT_DECAY * w;
    return w;
}

export void update_gradient( gradient_t& grad )
{
    grad.oldgrad = (grad.grad + grad.oldgrad * MOMENTUM);
}


export struct FeatureExtent {
    int x{}, y{}, z{};
};

export template<typename T>
struct tensor_t {
    Tensor<T, 3> data_;

    FeatureExtent size;

    tensor_t( int x, int y, int z ) {
        data_ = Tensor<T, 3>( z, x, y);
        size.x = x, size.y = y, size.z = z;
    }

    tensor_t( const tensor_t& other ) {
        data_ = other.data_;
        this->size = other.size;
    }

    tensor_t<T> operator + ( tensor_t<T>& other ) {
        tensor_t<T> ret(size.x, size.y, size.z);
        ret.data_ = data_ + other.data_;
        return ret;
    }

    tensor_t<T> operator - ( tensor_t<T>& other ) {
        tensor_t<T> ret(size.x, size.y, size.z);
        ret.data_ = data_ - other.data_;
        return ret;
    }

    T& operator()( int x, int y, int z ) {
        return data_[z, x, y];
    }
    T& operator[](int x, int y, int z) {
        return data_[z, x, y];
    }

    T& get( int x, int y, int z ) {
        return data_[z, x, y];
    }

};

export Tensor<float, 3> clone(tensor_t<float> &in) {
    Tensor<float, 3> ret(in.size.z, in.size.x, in.size.y);
    for (int z = 0; z < in.size.z; z++) {
        for (int x = 0; x < in.size.x; x++) {
            for (int y = 0; y < in.size.y; y++) {
                ret[z, x, y] = in(x, y, z);
            }
        }
    }
    return ret;
}

export tensor_t<float> clone(Tensor<float, 3> &in) {
    tensor_t<float> ret(in.extent(1), in.extent(2), in.extent(0));
    for (int z = 0; z < in.extent(0); z++) {
        for (int x = 0; x < in.extent(1); x++) {
            for (int y = 0; y < in.extent(2); y++) {
                ret(x, y, z) = in[z, x, y];
            }
        }
    }
    return ret;

}


export enum class layer_type
{
    conv,
    fc,
    relu,
    pool,
    dropout_layer
};

export class layer_t
{
public:
    layer_type type;
    tensor_t<float> grads_in;
    tensor_t<float> in;
    tensor_t<float> out;
    Tensor<float, 3> input, output;
    layer_t( layer_type type_, FeatureExtent in_size, FeatureExtent out_size ):
        type( type_ ),
        in( in_size.x, in_size.y, in_size.z ),
        grads_in( in_size.x, in_size.y, in_size.z ),
        out( out_size.x, out_size.y, out_size.z )
    {
    }
    virtual ~layer_t(){}
    virtual void activate( tensor_t<float>& in )=0;
    virtual void fix_weights()=0;
    virtual void calc_grads( tensor_t<float>& grad_next_layer )=0;
};

export class conv_layer_t: public layer_t {
public:
	// std::vector<tensor_t<float>> filters;
    Tensor<float, 3> gradient;
    Tensor<float, 4> filters;
    Tensor<gradient_t, 4> filters_grads;

	// std::vector<tensor_t<gradient_t>> filter_grads;
	uint16_t stride;
	uint16_t extend_filter;

	// 卷积层构造仅初始化卷积核
	conv_layer_t( uint16_t stride, uint16_t extend_filter, uint16_t number_filters, FeatureExtent in_size )
		:
		layer_t( layer_type::conv, in_size, {(in_size.x - extend_filter) / stride + 1, (in_size.y - extend_filter) / stride + 1, number_filters} )
	{
		this->stride = stride;
		this->extend_filter = extend_filter;
	    filters = Tensor<float, 4>(number_filters, in_size.z, extend_filter, extend_filter);

		int N = extend_filter * extend_filter * in_size.z;
	    filters.foreach([=](auto &val) {
	        val = 1.0f / N * rand() / 2147483647.0;
	    });
	    filters_grads = Tensor<gradient_t, 4>(number_filters, in_size.z, extend_filter, extend_filter);

	    input = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
	    FeatureExtent out_size = {(in_size.x - extend_filter) / stride + 1, (in_size.y - extend_filter) / stride + 1, number_filters};
	    output = Tensor<float, 3>( out_size.z, out_size.x, out_size.y );
	    gradient = Tensor<float, 3> (in_size.z, in_size.x, in_size.y);

	}

	FeatureExtent map_to_input( FeatureExtent out, int z )
	{
		return {out.x * stride, out.y * stride, z};
	}

	struct range_t
	{
		int min_x, min_y, min_z;
		int max_x, max_y, max_z;
	};

	int GET_R( float f, int max, bool lim_min )
	{
		if ( f <= 0 ) return 0;
		max -= 1;
		if ( f >= max ) return max;
		if ( lim_min ) return ceil( f );
		else return floor( f );
	}

	range_t map_to_output( int x, int y )
	{
		float a = x, b = y;
		return { GET_R( (a - extend_filter + 1) / stride, out.size.x, true ), GET_R( (b - extend_filter + 1) / stride, out.size.y, true ), 0, GET_R( a / stride, out.size.x, false ), GET_R( b / stride, out.size.y, false ), (int)out.size.z - 1, };
	}

	void activate( tensor_t<float>& in ) override
	{
		this->in = in;
	    input = clone(in);
		// 使用每个卷积核进行卷积
		for ( int n = 0; n < filters.extent(0); n++ )
		{

			// 对于输出的每个像素
			for ( int x = 0; x < output.extent(1); x++ )
				for ( int y = 0; y < output.extent(2); y++ )
				{
					FeatureExtent mapped = map_to_input( { (uint16_t)x, (uint16_t)y, 0 }, 0 );
					float sum = 0;
					for ( int i = 0; i < extend_filter; i++ )
						for ( int j = 0; j < extend_filter; j++ )
							for ( int z = 0; z < input.extent(0); z++ )
								sum += filters[n, z, i, j] * input[z, mapped.x + i, mapped.y + j];
				    output[n, x, y] = sum;
				}
		}
	    out = clone(output);
	}

	void fix_weights() override
	{
		for ( int a = 0; a < filters.extent(0); a++ )
			for ( int i = 0; i < extend_filter; i++ )
				for ( int j = 0; j < extend_filter; j++ )
					for ( int z = 0; z < input.extent(0); z++ )
					{
						float& w = filters[a, z, i, j];
						gradient_t& grad = filters_grads[a, z, i, j];
						w = update_weight( w, grad );
						update_gradient( grad );
					}
	}

	void calc_grads( tensor_t<float>& grad_next_layer ) override
	{
	    auto next_gradient = clone(grad_next_layer);

		// 卷积核梯度每次都初始化
		// 卷积核有n个，每个都是一个三维张量
		for ( int k = 0; k < filters_grads.extent(0); k++ )
			for ( int i = 0; i < extend_filter; i++ )
				for ( int j = 0; j < extend_filter; j++ )
					for ( int z = 0; z < input.extent(0); z++ )
						filters_grads[k, z, i, j].grad = 0;

		// 遍历当前图像
		for ( int x = 0; x < input.extent(1); x++ )
			for ( int y = 0; y < input.extent(2); y++ )
			{
				// 对于输入的每个像素，在卷积后输出的数据中，都有一个或者多个像素点使用了这个输入的像素
				// map_to_output 获取 使用了这个输入像素的 输出像素的坐标范围
				range_t rn = map_to_output( x, y );

				// 卷积操作仅操作了xy 坐标
				// in.size.z 是通道数量
				// 配合上面的xy, 这三个循环遍历了整张图片
				for ( int z = 0; z < input.extent(0); z++ ) {

					// 对于每个特征值， 有与之相对的 xyz范围
					float sum_error = 0;
					//out[i, j, k] -> in[x, y, z] 有贡献的位置
					for ( int i = rn.min_x; i <= rn.max_x; i++ )
					{
						int minx = i * stride;
						for ( int j = rn.min_y; j <= rn.max_y; j++ )
						{
							int miny = j * stride;
							for ( int k = rn.min_z; k <= rn.max_z; k++ )
							{
								//贡献的系数 -> 第k个核作用 out[ i, j, k] 对应的in区域，in[x, y, z] 的系数
								int K = filters[k, z ,x - minx, y - miny];
								//系数 * 偏导
								sum_error += K * next_gradient[k, i, j];
								//卷积核 grad 同理
								filters_grads[k, z, x-minx, y-miny].grad += input[z, x, y] * next_gradient[k, i, j];
							}
						}
					}
					//更新
				    gradient[z, x, y] = sum_error;
				}
			}
	    grads_in = clone(gradient);
	}
};


// 非线性激励层
export class relu_layer_t: public layer_t {

    Tensor<float, 3> input_;
    Tensor<float, 3> output_;
    std::shared_ptr<Activation> action_ = std::make_shared<Relu>();
    Tensor<float, 3> gradient;

public:

    relu_layer_t( FeatureExtent in_size )
        :
        layer_t( layer_type::relu, in_size, in_size ) {

        // 激活层尺寸不变
        input_ = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
        output_ = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
        gradient = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
    }

    void activate( tensor_t<float>& in ) override
    {
        this->in = in;
        input_ = clone(in);

        for ( int i = 0; i < input_.extent(1); i++ )
            for ( int j = 0; j < input_.extent(2); j++ )
                for ( int z = 0; z < input_.extent(0); z++ )
                    output_[z, i, j] = action_->action(input_[z, i, j]);

        out = clone(output_);
    }

    void fix_weights() override {}

    void calc_grads( tensor_t<float>& grad_next_layer ) override {
        auto next_gradient = clone(grad_next_layer);

        for ( int i = 0; i < in.size.x; i++ )
            for ( int j = 0; j < in.size.y; j++ )
                for ( int z = 0; z < in.size.z; z++ )
                    gradient[z, i, j] = action_->deaction(input_[z, i, j]) * next_gradient[z, i, j];
        grads_in = clone(gradient);
    }
};


export class pool_layer_t: public layer_t {

    Tensor<float, 3> input_;
    Tensor<float, 3> output_;
    Tensor<float, 3> gradient;
public:
	uint16_t stride;
	uint16_t extend_filter;

	pool_layer_t( uint16_t stride_, uint16_t extend_filter_, FeatureExtent in_size )
		:
		stride(stride_),
		extend_filter(extend_filter_),
		layer_t( layer_type::pool, in_size, {(in_size.x - extend_filter_) / stride_ + 1, (in_size.y - extend_filter_) / stride_ + 1, in_size.z} )
	{
	    input_ = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
	    FeatureExtent out_size = {(in_size.x - extend_filter_) / stride_ + 1, (in_size.y - extend_filter_) / stride_ + 1, in_size.z};
	    output_ = Tensor<float, 3>( out_size.z, out_size.x, out_size.y );

	    gradient = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
	}

	FeatureExtent map_to_input( FeatureExtent out, int z )
	{
		return {out.x * stride, out.y * stride, z};
	}

	struct range_t
	{
		int min_x, min_y, min_z;
		int max_x, max_y, max_z;
	};

	int GET_R( float f, int max, bool lim_min )
	{
		if ( f <= 0 ) return 0;
		max -= 1;
		if ( f >= max ) return max;
		if ( lim_min ) return ceil( f );
		else return floor( f );
	}

	range_t map_to_output( int x, int y )
	{
		float a = x, b = y;
		return {
			GET_R( (a - extend_filter + 1) / stride, out.size.x, true ),
			GET_R( (b - extend_filter + 1) / stride, out.size.y, true ),
			0,
			GET_R( a / stride, out.size.x, false ),
			GET_R( b / stride, out.size.y, false ), (int)out.size.z - 1,
		};
	}

	void activate( tensor_t<float>& in ) override
	{
		this->in = in;
	    input_ = clone(in);

		for ( int x = 0; x < output_.extent(1); x++ )
			for ( int y = 0; y < output_.extent(2); y++ )
				for ( int z = 0; z < output_.extent(0); z++ )
				{
					FeatureExtent mapped = map_to_input( { (uint16_t)x, (uint16_t)y, 0 }, 0 );
					float maxx = -FLT_MAX;
					for ( int i = 0; i < extend_filter; i++ )
						for ( int j = 0; j < extend_filter; j++ )
						{
							float v = input_[z, mapped.x + i, mapped.y + j];
							if ( v > maxx ) maxx = v;
						}
				    output_[z, x, y] = maxx;
				}
	    out = clone(output_);
	}

	void fix_weights() override {}

	void calc_grads( tensor_t<float>& grad_next_layer ) override
	{
	    auto next_gradient = clone(grad_next_layer);

		// 对于输入的[x, y]
		for ( int x = 0; x < input_.extent(1); x++ )
			for ( int y = 0; y < input_.extent(2); y++ )
			{
				// 找到x y对应的什么范围
				// 得到的range是 该点对应的 在池化层中的位置
				range_t rn = map_to_output( x, y );

				// 池化是逐层的，因此z应该不参与到range的运算
				// in.size.z 是通道数量
				for ( int z = 0; z < input_.extent(0); z++ )
				{
					float sum_error = 0;
					//out[i, j, z] 是 in[x, y, z] 可能有贡献的位置，贡献的系数是 1 或者 0

					// i, j 对应的是池化窗口中的坐标
					for ( int i = rn.min_x; i <= rn.max_x; i++ ) {
						for ( int j = rn.min_y; j <= rn.max_y; j++ ) {
							int is_max = in( x, y, z ) == out( i, j, z ) ? 1 : 0;
							//偏导 * 系数
							sum_error += is_max * next_gradient[z, i, j];
						}
					}
				    gradient[z, x, y] = sum_error;
				}
			}
	    grads_in = clone(gradient);
	}
};
export class fc_layer_t: public layer_t {
    std::shared_ptr<Activation> action_ = std::make_shared<Sigmoid>();
    Tensor<float, 3> input_, output_;
    Tensor<float, 3> gradient;
public:
	Vector<float> fc_in, fc_out;

	// 权值矩阵 => 三维张量
	// tensor_t<float> weights;
    Tensor<float, 4> weights;

	// 梯度, 此处全连接层的梯度是一个vector<梯度>
	Vector<gradient_t> gradients;

	fc_layer_t( FeatureExtent in_size, int out_size )
		:
		layer_t( layer_type::fc, in_size, {out_size, 1, 1} ),
        weights(out_size, in_size.z, in_size.x, in_size.y)
	{
		fc_in = Vector<float>( out_size );
	    fc_out = Vector<float>( out_size );

		gradients = Vector<gradient_t>( out_size );
	    gradient = Tensor<float, 3> (in_size.z, in_size.x, in_size.y);

	    input_ = Tensor<float, 3>( in_size.z, in_size.x, in_size.y );
	    output_ = Tensor<float, 3>( 1, 1, out_size );

	    random_tensor(weights, -1, 1);
	}



	void activate( tensor_t<float>& in ) override
	{
		// 前向传播的输入是一个三维张量
		this->in = in;

	    Tensor<float, 3> input = clone(in);
	    Tensor<float, 3> output(input.extent(0), input.extent(1), input.extent(2));

		float now = 0;
		for ( int cnt = 0; cnt < out.size.x; cnt++, now=0 )
		{
			for ( int i = 0; i < input.extent(1); i++ )
				for ( int j = 0; j < input.extent(2); j++ )
					for ( int z = 0; z < input.extent(0); z++ )
					    now += input[z, i, j] * weights[cnt, z, i, j];

			// 三维张量和权值张量做点乘，展成一维张量
			fc_in[cnt] = now;
			// 输入通过激活函数做输出, 因此输出也是一维张量
			// 且二者尺度相同
		    fc_out[cnt] = action_->action( now );
		}
	}

	// 更新权重，根据本层中此前计算的梯度(grad)
	void fix_weights() override
	{

		// 对于每一个输出神经元
		for ( int n = 0; n < out.size.x; n++ )
		{
			gradient_t& grad = gradients[n];
			for ( int i = 0; i < in.size.x; i++ ) {
				for ( int j = 0; j < in.size.y; j++ ) {
					for ( int k = 0; k < in.size.z; k++ ) {
						// float& w = weights( id( i, j, k ), n, 0 );
					    auto &w = weights[n, k, i, j];
						w = update_weight( w, grad, in( i, j, k ) );
					}

				}

			}

			update_gradient( grad );
		}
	}

	void calc_grads( tensor_t<float>& grad_next_layer ) override {  }
	// 下一层保存的梯度是一个三维张量
	void calc_grads( Vector<float>& grad_next_layer ) {
	    gradient.foreach([](auto &val) {
	        val = 0;
	    });


		// 此处全连接层输入的是 均方误差损失函数得到的 (y_ - y)
		// grads_in 中保存的是下一层使用的梯度
		// 梯度综合了全部的权值矩阵
		for ( int n = 0; n < out.size.x; n++ )
		{
			// 全连接层作为最后一层，起初保存的梯度仅和输入输出 一维张量的大小相同
			// 此处的gradient是fc层独有的，尺寸是一维的
			// 在构造函数中创建
			gradient_t& grad = gradients[n];

			// grad 中的梯度用来更新权重
			grad.grad = grad_next_layer[ n ] * action_->deaction( fc_in[n] );

			// 此处将原本的一维梯度通过权值张量转为三维梯度
			for ( int i = 0; i < in.size.x; i++ )
				for ( int j = 0; j < in.size.y; j++ )
					for ( int k = 0; k < in.size.z; k++ )
					    gradient[k, i, j] += grad.grad * weights[n, k, i, j];
						// grads_in( i, j, k ) += grad.grad * weights( id( i, j, k ), n, 0 );
		}

	    grads_in = clone(gradient);
	}

};

export class Model
{
public:
	Model(){}

	void add_conv( uint16_t stride, uint16_t extend_filter, uint16_t number_filters, FeatureExtent in_size )
	{
		conv_layer_t * layer = new conv_layer_t( stride, extend_filter, number_filters, in_size);
		layers.push_back( (layer_t*)layer );
	}

	void add_relu( FeatureExtent in_size )
	{
		relu_layer_t * layer = new relu_layer_t( in_size );
		layers.push_back( (layer_t*)layer );
	}

	void add_pool( uint16_t stride, uint16_t extend_filter, FeatureExtent in_size )
	{
		pool_layer_t * layer = new pool_layer_t( stride, extend_filter, in_size );
		layers.push_back( (layer_t*)layer );
	}

	void add_fc( FeatureExtent in_size, int out_size )
	{
		fc_layer_t * layer = new fc_layer_t( in_size, 10 );
		layers.push_back( (layer_t*)layer );
	}

	FeatureExtent& output_size()
	{
		return layers.back()->out.size;
	}

	int predict()
	{
		int ans = 0;
	    auto end_layer = dynamic_cast<fc_layer_t*>( layers.back() );
		for( int i = 0; i < 10; i++ )
			if( end_layer->fc_out[i] > end_layer->fc_out[ans] )
				ans = i ;
		return ans;
	}

	tensor_t<float>& predict_info()
	{
		return layers.back()->out;
	}

	void forward( tensor_t<float>& data ) {
	    auto begin_layer = layers.front();
	    begin_layer->activate(data);
		for ( int i = 1; i < layers.size(); i++ )
			layers[i]->activate(layers[i - 1]->out);
	}

	float train( Tensor<float, 3> &data, Vector<float>& label )
	{

	    auto in_data = clone(data);
		// 前向传播
		forward( in_data );

		// 最后一层，目前为全连接层
		// 和最后的结果相减得到差值
	    auto end_label = dynamic_cast<fc_layer_t *>(layers.back());



	    auto vec_res_info = end_label->fc_out - label;




	    end_label->calc_grads( vec_res_info );
		for ( int i = layers.size() - 2; i >= 0; i-- )
			layers[i]->calc_grads( layers[i + 1]->grads_in);

		for ( int i = 0; i < layers.size(); i++ )
			layers[i]->fix_weights();


	    // 计算误差
		float err = 0;
		for ( int i = 0; i < 10; i++ )
		{
			float x = label[i] - vec_res_info[i];
			err += x*x ;
		}
		return sqrt(err) * 100;
	}


private:
	std::vector<layer_t*> layers;
};



uint32_t byteswap_uint32(uint32_t a)
{
	return ((((a >> 24) & 0xff) << 0) |
		(((a >> 16) & 0xff) << 8) |
		(((a >> 8) & 0xff) << 16) |
		(((a >> 0) & 0xff) << 24));
}

export struct case_t
{
	Tensor<float, 3> data;
    Vector<float> label;
};

export uint8_t* read_file( const char* szFile )
{
	std::ifstream file( szFile, std::ios::binary | std::ios::ate );
	std::streamsize size = file.tellg();
	file.seekg( 0, std::ios::beg );

	if ( size == -1 )
		return nullptr;

	uint8_t* buffer = new uint8_t[size];
	file.read( (char*)buffer, size );
	return buffer;
}

export std::vector<case_t> read_train_cases()
{
	std::vector<case_t> cases;

	uint8_t* train_image = read_file( "train-images.idx3-ubyte" );
	uint8_t* train_labels = read_file( "train-labels.idx1-ubyte" );

	uint32_t case_count = byteswap_uint32( *(uint32_t*)(train_image + 4) );

	for ( int i = 0; i < case_count; i++ )
	{
		case_t c
		{
			// image
			Tensor<float, 3>(1, 28, 28),
			// one hot labels
		Vector<float>( 10 )
		};

		uint8_t* img = train_image + 16 + i * (28 * 28);
		uint8_t* label = train_labels + 8 + i;

		for ( int x = 0; x < 28; x++ )
			for ( int y = 0; y < 28; y++ )
				c.data[0, x, y] = img[x + y * 28] / 255.f;

		for ( int b = 0; b < 10; b++ )
			c.label[ b] = *label == b ? 1.0f : 0.0f;

		cases.push_back( c );
	}
	delete[] train_image;
	delete[] train_labels;

	return cases;
}

export std::vector<case_t> read_test_cases()
{
	std::vector<case_t> cases;

	uint8_t* train_image = read_file( "t10k-images.idx3-ubyte" );
	uint8_t* train_labels = read_file( "t10k-labels.idx1-ubyte" );

	uint32_t case_count = byteswap_uint32( *(uint32_t*)(train_image + 4) );

	for ( int i = 0; i < case_count; i++ )
	{
		case_t c
		{
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
