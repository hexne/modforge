/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/12/10 22:32
*******************************************************************************/

export module modforge.tensor;
import std;

NAMESPACE_BEGIN

export
template <typename T, std::size_t Extents>
class Tensor;



/** @brief 一维向量：数据由 shared_ptr 持有，拷贝为浅拷贝（共享存储）
 *  @tparam T 元素类型
 */
export
template<typename T>
class Vector {
    std::shared_ptr<std::vector<T>> data_;

public:
    Vector() = default;
    /** @brief 构造长度为 n 的向量，元素值初始化为 T{}
     *  @param n 元素个数
     */
    explicit Vector(int n) : data_(std::make_shared<std::vector<T>>(n)) {  }
    /** @brief 从外部数组拷贝构造
     *  @param ptr 源数组首地址，为 nullptr 时得到长度 size 的零向量
     *  @param size 元素个数
     */
    Vector(T *ptr, std::size_t size) : data_(std::make_shared<std::vector<T>>(size)) {
        if (ptr && size)
            std::copy(ptr, ptr + size, data_->begin());
    }

    Vector(const Vector &) = default;
    Vector(Vector &&) = default;

    Vector &operator = (const Vector &) = default;
    Vector &operator = (Vector &&) = default;

    /** @brief 逐元素相减
     *  @param right 右操作数
     *  @return 新向量
     *  @throw std::runtime_error 两向量长度不等
     */
    Vector operator - (const Vector &right) const {
        if (data_->size() != right.data_->size())
            throw std::runtime_error("Vector size mismatch");

        Vector ret(data_->size());
        for (int i = 0;i < data_->size();i++)
            ret[i] =  data_->operator[](i) - right[i];

        return ret;
    }

    /** @brief 行向量左乘矩阵：(1×M) × (M×N) → (1×N)
     *  @param tensor 右矩阵，行数须等于本向量长度
     *  @return 长度 N 的结果向量
     *  @throw std::runtime_error 维度不匹配
     */
    Vector operator *(const Tensor<T, 2> &tensor) {
        if (data_->size() != tensor.extent(0)) {
            throw std::runtime_error(std::format("{}x{} cant * {}x{}", 1, data_->size(), tensor.extent(0), tensor.extent(1)));
        }

        Vector ret(tensor.extent(1));

        for (int y = 0; y < tensor.extent(1); ++y) {
            T res{};
            for (int x = 0; x < tensor.extent(0); ++x) {
                res += (*data_)[x] * tensor[x, y];
            }
            ret[y] = res;
        }
        return ret;
    }
    /** @brief 原地右乘矩阵，等价于 *this = *this * tensor
     *  @return *this
     */
    Vector &operator *=(const Tensor<T, 2> &tensor) {
        auto ret = operator*(tensor);
        *this = ret;
        return *this;
    }

	/** @brief 下标访问（不检查越界）
	 *  @param index 元素下标
	 *  @return 元素引用
	 */
	T& operator[](std::size_t index) {
		return (*data_)[index];
	}
    /** @brief 常量下标访问
     *  @return 元素副本
     */
    T operator[](std::size_t index) const {
        return (*data_)[index];
    }

    /** @brief 元素个数；默认构造时为 0
     */
    std::size_t size() const {
        if (data_)
            return data_->size();
        return 0;
    }

    /** @brief 依次对每个元素回调 func；未初始化时为空操作
     *  @param func 元素回调，接收可修改的引用
     */
    void foreach(std::function<void(T &)> func) {
        if (!data_)
            return;
        for (auto &val : *data_)
            func(val);
    }

    /** @brief 从二进制流读入：先 4 字节 int 长度，再连续的元素数据
     *  @param in 输入流
     */
    void read(std::istream &in) {
        int size{};
        in.read(reinterpret_cast<char *>(&size), sizeof(size));
        data_ = std::make_shared<std::vector<T>>(size);
        if (size > 0)
            in.read(reinterpret_cast<char *>(data_->data()), sizeof(T) * static_cast<std::size_t>(size));
    }
    /** @brief 按与 read 相同的二进制格式写出
     *  @param out 输出流
     */
    void write(std::ostream &out) const {
        int size = data_ ? static_cast<int>(data_->size()) : 0;
        out.write(reinterpret_cast<const char *>(&size), sizeof(size));
        if (size > 0)
            out.write(reinterpret_cast<const char *>(data_->data()), sizeof(T) * static_cast<std::size_t>(size));
    }

    /** @brief 流输入，等价 vec.read(in) */
    friend std::istream &operator >> (std::istream &in, Vector &vec) {
        vec.read(in);
        return in;
    }
    /** @brief 流输出，等价 vec.write(out) */
    friend std::ostream &operator << (std::ostream &out, Vector &vec) {
        vec.write(out);
        return out;
    }

};

/** @brief 任意秩张量：数据由 shared_ptr 持有，视图为 layout_stride 的 mdspan
 *  @tparam T 元素类型
 *  @tparam Extents 秩（维度数）
 *  @note 拷贝为浅拷贝，拷贝后与原张量共享数据；由 from_view 构造的张量不拥有数据
 */
export
template <typename T,std::size_t Extents>
class Tensor {
    std::shared_ptr<std::vector<T>> data_;
    std::mdspan<T, std::dextents<std::size_t, Extents>, std::layout_stride> view_;

    template<typename ... Args>
    constexpr int mul(Args && ...args) {
        return (... * args);
    }

    template<typename U, typename ... Args>
    [[nodiscard]]
    auto& access_vector(std::vector<U>& vec, std::size_t first, Args &&...index) {
        if constexpr (sizeof ...(index) >= 1)
            return access_vector(vec[first], index ...);
        else
            return vec[first];
    }

    template<std::size_t extent>
    auto to_vector_impl() {
        if constexpr (extent == 1) {
            return std::vector<T>(view_.extent(Extents - 1));
        }
        else {
            return std::vector(view_.extent(Extents - extent), to_vector_impl<extent - 1>());
        }
    }

    template<typename Vector,typename ... Index>
    void copy_to_vector_impl(Vector &vec, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            access_vector(vec, index...) = view_[index ...];
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                copy_to_vector_impl(vec, index..., i);
            }
        }
    }
    // - +
    template<typename Mdspan,typename OP, typename ... Index>
    void traversal_mdspan_impl(Mdspan &mdspan1, Mdspan &mdspan2, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            view_[index ...] = op(mdspan1[index ...], mdspan2[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                traversal_mdspan_impl(mdspan1, mdspan2, op, index..., i);
            }
        }
    }
    // -= +=
    template<typename Mdspan,typename OP, typename ... Index>
    void traversal_mdspan_impl(Mdspan &mdspan, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            op(view_[index ...], mdspan[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                traversal_mdspan_impl(mdspan, op, index..., i);
            }
        }
    }

    // foreach
    template <typename Mdspan, typename OP, typename ... Index>
    void foreach_impl(Mdspan &mdspan, OP &&op, Index &&...index) {
        if constexpr (sizeof...(index) == Extents) {
            op(view_[index ...]);
        }
        else {
            for (int i = 0; i < view_.extent(sizeof...(index)); i++) {
                foreach_impl(mdspan, op, index..., i);
            }
        }

    }

    template <std::size_t N, typename ... Index>
    void mul_impl(Tensor &result, const Tensor &other, Index ...index) const {
        // 此处计算乘法
        if constexpr (N == Extents - 2) {
            const std::size_t n = view_.extent(Extents - 2); // 结果行数
            const std::size_t p = other.view_.extent(Extents - 1); // 结果列数
            const std::size_t k = view_.extent(Extents - 1); // 内积维度

            for (std::size_t i = 0; i < n; ++i) {
                for (std::size_t j = 0; j < p; ++j) {
                    T sum{};
                    for (std::size_t l = 0; l < k; ++l) {
                        sum += view_[index..., i, l] * other.view_[index..., l, j];
                    }
                    result.view_[index..., i, j] = sum;
                }
            }
        }
        else {
            for (int i = 0;i < view_.extent(N); ++i) {
                mul_impl<N + 1>(result, other, index..., i);
            }

        }


    }

    template<std::size_t... I>
    void create_tensor(std::vector<int>& dims, std::index_sequence<I...>) {
        Tensor tmp(dims[I]...);
        *this = std::move(tmp);
    }
public:
    Tensor() = default;

    /** @brief 按各维长度构造，元素值初始化为 T{}
     *  @param args 各维长度，个数须等于 Extents
     */
    Tensor(auto && ...args) requires (sizeof ...(args) == Extents) {
        data_ = std::make_shared<std::vector<T>>(mul(args ...));
        view_ = std::mdspan(data_->data(), args ...);
    }

    // 不拥有数据的所有权，为了让view使用更方便，该接口留给view使用
    /** @brief 用外部数据构造视图，不拥有数据所有权
     *  @param data 外部数据首地址，其生命周期须长于返回的张量
     *  @param args 各维长度
     */
    static Tensor from_view(const T* data, auto && ...args) requires (sizeof ...(args) == Extents) {
        Tensor<T, sizeof ...(args)> ret;
        ret.view_ = std::mdspan(data, args ...);
        return ret;
    }
    /** @brief 用外部数据与给定的步长映射构造视图，不拥有数据所有权
     */
    static Tensor from_view(const T* data, std::layout_stride::mapping<std::dextents<std::size_t, Extents>> mapping) {
        Tensor ret;
        ret.view_ = std::mdspan(data, mapping);
        return ret;
    }
    /** @brief 复制已有张量的视图（共享数据与形状，不拥有数据）
     */
    static Tensor<T, Extents> from_view(Tensor<T, Extents> &tensor) {
        Tensor ret;
        ret.view_ = tensor.view_;
        int x = ret.view_.extent(0);
        int y = ret.view_.extent(1);
        return ret;
    }

    /** @brief 从数组拷入 count 个元素
     *  @param data 源数组首地址
     *  @param count 元素个数
     *  @param args 各维长度
     */
    Tensor(const T *data, std::size_t count, auto && ...args) requires (sizeof ...(args) == Extents) {
        data_ = std::make_shared<std::vector<T>>(count);
        std::copy(data, data + count, data_->begin());
        view_ = std::mdspan(data_->data(), args ...);
    }

    /** @brief 同上，接受 shared_ptr 管理的数组
     */
    Tensor(std::shared_ptr<T[]> ptr,std::size_t count, auto && ...args) requires (sizeof ...(args) == Extents)
                : Tensor(ptr.get(), count, std::forward<decltype(args)>(args) ...) {

    }

    /** @brief 从 vector 拷入数据
     */
    explicit Tensor(const std::vector<T> &vec, auto && ...args) requires (sizeof ...(args) == Extents) : Tensor(std::data(vec), vec.size(), args ...) {  }

    /** @brief 接管给定的数据指针，并按给定步长映射建立视图
     */
    Tensor(std::shared_ptr<std::vector<T>> vec, std::layout_stride::mapping<std::dextents<std::size_t, Extents>> mapping) : data_(vec), view_(data_->data(), mapping) {  }

    /** @brief 从初始化列表拷入数据
     */
    Tensor(const std::initializer_list<T> &vec, auto && ...args) requires (sizeof ...(args) == Extents) : Tensor(std::data(vec), vec.size(), args ...) {  }

    Tensor(const Tensor &) = default;
    Tensor& operator = (const Tensor &) = default;

    Tensor(Tensor &&) = default;
    Tensor& operator = (Tensor &&) = default;

    /** @brief 从二进制流读入：4 字节秩 + 每维 4 字节长度 + 按视图顺序的全部元素
     *  @param in 输入流
     *  @note 流中的秩须与 Extents 一致，否则维度重建结果无意义
     */
    void read(std::istream &in) {
        int rank;
        in.read(reinterpret_cast<char*>(&rank),sizeof(int));
        std::vector<int> dims(rank);
        for (int i = 0;i < rank; ++i) {
            in.read(reinterpret_cast<char*>(&dims[i]),sizeof(int));
        }

        create_tensor(dims, std::make_index_sequence<Extents>{});
        this->foreach([&](auto &val) {
            in.read(reinterpret_cast<char*>(&val),sizeof(val));
        });

    }
    /** @brief 按与 read 相同的二进制格式写出
     *  @param out 输出流
     */
    void write(std::ostream &out) {
        // 维度
        int rank = view_.rank();
        out.write(reinterpret_cast<const char*>(&rank), sizeof(rank));
        // 依次保存维度
        for (int i = 0; i < rank; ++i) {
            int cur_size = view_.extent(i);
            out.write(reinterpret_cast<const char*>(&cur_size), sizeof(cur_size));
        }

        // 保存剩下的数据
        this->foreach([&] (auto &val){
            out.write(reinterpret_cast<char*>(&val), sizeof(val));
        });

    }

    /** @brief 流输入，等价 tensor.read(in) */
    friend std::istream &operator >> (std::istream &in, Tensor &tensor) {
        tensor.read(in);
        return in;
    }
    /** @brief 流输出，等价 tensor.write(out) */
    friend std::ostream &operator << (std::ostream &out, Tensor &tensor) {
        tensor.write(out);
        return out;
    }


    /** @brief 深拷贝：数据独立，形状与步长不变
     */
    Tensor copy() const {
        auto ptr = std::make_shared<std::vector<T>>(data_->size());
        std::copy(data_->begin(), data_->end(), ptr->begin());
        return Tensor(ptr, view_.mapping());
    }

    /** @brief 只拷贝同尺寸的空存储，元素为 T{}
     */
    Tensor copy_size() const {
        auto ptr = std::make_shared<std::vector<T>>(data_->size());
        return Tensor(ptr, view_.mapping());
    }

    /** @brief 第 extent 维的长度
     */
    [[nodiscard]] std::size_t extent(std::size_t extent) const {
        return view_.extent(extent);
    }

    /** @brief 秩，恒等于 Extents
     */
    [[nodiscard]] std::size_t rank() const {
        return view_.rank();
    }

    /** @brief 按视图顺序遍历所有元素并回调
     *  @param func 元素回调，接收可修改的引用
     */
    void foreach(std::function<void (T &)> func) {
        foreach_impl(view_, func);
    }

    /** @brief 导出为嵌套 vector，嵌套层数等于 Extents
     *  @param only_size true 时只构造相同形状、元素为 T{} 的嵌套 vector
     *  @return 最外层 vector 的维度等于第 0 维长度
     */
    auto to_vector(bool only_size = false) const {
        auto vec = to_vector_impl<Extents>();
        if (!only_size)
            copy_to_vector_impl(vec);
        return vec;
    }

    /** @brief 多维下标访问（不检查越界）
     *  @param index 各维下标，个数须等于 Extents
     *  @return 元素引用
     */
    T& operator [] (auto && ...index) requires (sizeof ...(index) == Extents) {
        return view_[index ...];
    }

    /** @brief 常量多维下标访问
     *  @return 元素副本
     */
    T operator [] (auto && ...index) const requires (sizeof ...(index) == Extents) {
        return view_[index ...];
    }

    /** @brief 逐维比较形状是否完全一致
     *  @param other 待比较的张量
     *  @return 全部维度相同则为 true
     */
    constexpr bool check_size(const Tensor &other) const {
        for (int i = 0;i < view_.rank(); ++i)
            if (view_.extent(i) != other.view_.extent(i))
                return false;
        return true;
    }
    /** @brief 逐元素相加
     *  @throw std::runtime_error 形状不一致
     */
    Tensor operator + (const Tensor &other) const {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        auto ret = other.copy_size();
        ret.traversal_mdspan_impl(view_, other.view_, [] (auto val1, auto val2){
            return val1 + val2;
        });
        return ret;
    }
    /** @brief 逐元素相减
     *  @throw std::runtime_error 形状不一致
     */
    Tensor operator - (const Tensor &other) const {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        auto ret = other.copy_size();
        ret.traversal_mdspan_impl(view_, other.view_, [] (auto val1, auto val2){
            return val1 - val2;
        });
        return ret;
    }

    /** @brief 原地逐元素相加
     *  @throw std::runtime_error 形状不一致
     */
    Tensor &operator += (const Tensor &other) {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        traversal_mdspan_impl(other.view_, [] (auto &val1, auto &val2){
            return val1 += val2;
        });
        return *this;
    }
    /** @brief 原地逐元素相减
     *  @throw std::runtime_error 形状不一致
     */
    Tensor &operator -= (const Tensor &other) {
        if (!check_size(other))
            throw std::runtime_error("Tensor size is different");

        traversal_mdspan_impl(other.view_, [] (auto &val1, auto &val2){
            return val1 -= val2;
        });
        return *this;
    }


    /** @brief 批量矩阵乘：对前 Extents-2 维逐块做矩阵乘
     *  @param other 右张量：秩须相同、前 Extents-2 维长度须相同，
     *               且本张量最后一维须等于 other 的倒数第二维
     *  @return 新张量，最后两维为 [本张量倒数第二维, other 最后一维]
     *  @throw std::runtime_error 秩或维度不匹配
     */
    Tensor operator * (const Tensor &other) const {
        if (view_.rank() != other.view_.rank())
            throw std::runtime_error("Tensor rank is different");

        for (int i = 0;i < view_.rank() - 2; ++i) {
            if (view_.extent(i) != other.view_.extent(i))
                throw std::runtime_error("Tensor size is different");
        }

        if (view_.extent(Extents - 1) != other.view_.extent(Extents - 2))
            throw std::runtime_error("Tensor size is different");


        std::array<std::size_t, Extents> new_extents;
        for (int i = 0;i < Extents - 2; ++i)
            new_extents[i] = view_.extent(i);
        new_extents[Extents - 2] = view_.extent(Extents - 2);
        new_extents[Extents - 1] = other.view_.extent(Extents - 1);
        auto create = [&]<std::size_t ...Index>(std::index_sequence<Index...>) {
            return Tensor<T, Extents>(new_extents[Index]...);
        };
        auto result = create(std::make_index_sequence<Extents>());

        mul_impl<0>(result, other);
        return result;
    }
    /** @brief 原地矩阵乘，等价于 *this = *this * other
     */
    Tensor& operator *= (const Tensor &other) {
        auto res = operator*(other);
        *this = std::move(res);
        return *this;
    }

    /** @brief 交换最后两维：原地、零拷贝，只改变视图的形状与步长
     *  @return *this
     *  @note 仅支持秩 2 与秩 3；数据本身不移动
     */
    Tensor& transpose() {
        static_assert(Extents >= 2, "Transpose requires at least 2 dimensions");

        // 新的步长
        std::array<std::size_t, Extents> strides;
        for (std::size_t i = 0; i < Extents - 2; ++i)
            strides[i] = view_.mapping().stride(i);
        strides[Extents - 2] = view_.mapping().stride(Extents - 1);
        strides[Extents - 1] = view_.mapping().stride(Extents - 2);

        // 新的维度
        std::array<std::size_t, Extents> extents;
        for (std::size_t i = 0; i < Extents; ++i)
            extents[i] = view_.extent(i);
        std::swap(extents[Extents - 2], extents[Extents - 1]);

        // 构造 extents 对象
        auto new_extents = [&]{
            if constexpr (Extents == 1) {
                return std::dextents<std::size_t, 1>{extents[0]};
            } else if constexpr (Extents == 2) {
                return std::dextents<std::size_t, 2>{extents[0], extents[1]};
            } else if constexpr (Extents == 3) {
                return std::dextents<std::size_t, 3>{extents[0], extents[1], extents[2]};
            } else {
                static_assert(Extents <= 3, "Only support up to 3 dimensions");
                return std::dextents<std::size_t, Extents>{};
            }
        }();

        view_ = std::mdspan(view_.data_handle(),
                           std::layout_stride::mapping(new_extents, strides));

        return *this;
    }


};

template <typename T, typename ...Args>
Tensor(T *, std::size_t, Args ...) -> Tensor<T, sizeof...(Args)>;

template <typename T, typename ...Args>
Tensor(const std::vector<T> &vec, Args ...) -> Tensor<T, sizeof...(Args)>;

template <typename T, typename ...Args>
Tensor(const std::initializer_list<T> &vec, Args ...) -> Tensor<T, sizeof...(Args)>;

NAMESPACE_END
