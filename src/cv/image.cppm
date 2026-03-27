/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/10/16 22:09
*******************************************************************************/

module;
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
export module modforge.image;
import std;
NAMESPACE_BEGIN
export class Image {
    template <typename T>
    static constexpr bool is_char_type =
        std::is_same_v<std::remove_cv_t<T>, char> ||
        std::is_same_v<std::remove_cv_t<T>, unsigned char>;

    template <typename T>
    bool open_from_memory(const T *ptr, std::size_t size) {
        if (!ptr or size == 0) {
            image_.release();
            return false;
        }
        auto begin = reinterpret_cast<const unsigned char *>(ptr);
        std::vector<unsigned char> bytes(begin, begin + size);
        image_ = cv::imdecode(bytes, cv::IMREAD_COLOR);
        return is_open();
    }

    cv::Mat image_;
public:
    Image() = default;
    explicit Image(const std::string &path) : image_(cv::imread(path)) { }

    template<typename T>
        requires is_char_type<T>
    Image(const T *prt, std::size_t size) {
        open_from_memory(prt, size);
    }

    Image(const Image & image) {
        image_ = image.image_;
    }
    Image operator = (const Image & image) {
        image_ = image.image_;
        return *this;
    }
    Image clone() const {
        Image ret;
        ret.image_ = image_.clone();
        return ret;
    }

    // 打开
    bool open(const std::string &path) {
        image_ = cv::imread(path);
        return is_open();
    }
    template <typename T>
        requires is_char_type<T>
    bool open(const T *ptr, std::size_t size) {
        return open_from_memory(ptr, size);
    }
    bool is_open() const {
        return !image_.empty();
    }

    // 保存
    void save(const std::string &path) const {
        cv::imwrite(path, image_);
    }

    int channels() const {
        if (image_.empty())
            return 0;
        return image_.channels();
    }

    int width() const {
        return image_.cols;
    }
    int height() const {
        return image_.rows;
    }
    std::tuple<int, int> size() const {
        return {width(), height()};
    }
    std::tuple<int, int, int> extent() const {
        return {channels(), width(), height()};
    }

    Image& width(int width) {
        cv::resize(image_,image_,cv::Size(width,image_.rows));
        return *this;
    }
    Image& height(int height) {
        cv::resize(image_,image_,cv::Size(image_.cols,height));
        return *this;
    }
    Image& size(int width, int height) {
        cv::resize(image_, image_, cv::Size(width, height));
        return *this;
    }

    Image& rotate(int angle) {
        cv::Point2f center(image_.cols / 2, image_.rows / 2);
        cv::Mat matrix = cv::getRotationMatrix2D(center, angle, 1.0);
        cv::warpAffine(image_, image_, matrix, image_.size());
        return *this;
    }
    Image& zoom(float multiple) {
        int new_width = image_.cols * multiple;
        int new_height = image_.rows * multiple;
        cv::resize(image_, image_, cv::Size(new_width, new_height));
        return *this;
    }

    Image& to_grayscale() {
        cv::cvtColor(image_, image_, cv::COLOR_BGR2GRAY);
        return *this;
    }
    Image& to_binary(int threshold) {
        cv::threshold(image_, image_, threshold, 255, cv::THRESH_BINARY);
        return *this;
    }

    // B G R
    std::vector<std::array<size_t, 256>> get_histogram_data() const {
        std::vector<std::array<size_t, 256>> ret;
        auto [z, x, y] = extent();
        for (int c = 0; c < z; c++) {
            std::array<size_t, 256> data{};
            for (int i = 0; i < x; i++) {
                for (int j = 0; j < y; j++) {
                    data[operator[](c, i, j)] ++;
                }
            }
            ret.emplace_back(data);
        }
        return ret;
    }


    // B G R
    uchar& operator[](int z, int x, int y) {
        if (image_.channels() == 1 && z != 0)
            throw std::out_of_range("channels is 1, and z is not 0");
        return image_.data[y * image_.step + x * image_.channels() + z];
    }

    const uchar& operator[](int z, int x, int y) const {
        if (image_.channels() == 1 && z != 0)
            throw std::out_of_range("channels is 1, and z is not 0");
        return image_.data[y * image_.step + x * image_.channels() + z];
    }

    // 显示
    void show(const std::string &window_name) const {
        cv::imshow(window_name, image_);
    }
    void show_and_wait(const std::string & window_name, int milliseconds = 0) const {
        show(window_name);
        cv::waitKey(milliseconds);
    }

    // 获取窗口句柄
    static auto get_window(const std::string &window_name) {
        return cvGetWindowHandle(window_name.data());
    }

    ~Image() = default;
};

NAMESPACE_END
