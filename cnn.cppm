/********************************************************************************
* @Author : hexne
* @Date   : 2025/05/21 16:27:03
********************************************************************************/
module;
#include <vector>
#include <fstream>
export module modforge.deep_learning.cnn;

import modforge.tensor;
import modforge.deep_learning.tools;



int32_t swap_endian(int32_t val) {
    return ((val >> 24) & 0xff) | ((val << 8) & 0xff0000) |
           ((val >> 8) & 0xff00) | ((val << 24) & 0xff000000);
}

export
auto load_dataset(const std::string &path_images, const std::string &path_labels) {
    std::vector<std::pair<Tensor<float, 3>, Vector<float>>> ret;

    std::ifstream images_in(path_images, std::ios::binary);
    if (!images_in) throw std::runtime_error("Failed to open images file: " + path_images);

    int32_t magic, num_images, rows, cols;
    images_in.read(reinterpret_cast<char*>(&magic), 4);
    images_in.read(reinterpret_cast<char*>(&num_images), 4);
    images_in.read(reinterpret_cast<char*>(&rows), 4);
    images_in.read(reinterpret_cast<char*>(&cols), 4);

    magic = swap_endian(magic);
    num_images = swap_endian(num_images);
    rows = swap_endian(rows);
    cols = swap_endian(cols);

    std::ifstream labels_in(path_labels, std::ios::binary);
    if (!labels_in) throw std::runtime_error("Failed to open labels file: " + path_labels);

    int32_t label_magic, label_count;
    labels_in.read(reinterpret_cast<char*>(&label_magic), 4);
    labels_in.read(reinterpret_cast<char*>(&label_count), 4);
    label_magic = swap_endian(label_magic);
    label_count = swap_endian(label_count);

    if (magic != 2051 || label_magic != 2049 || num_images != label_count)
        throw std::runtime_error("Invalid MNIST file format");

    std::vector<uint8_t> image_data(num_images * rows * cols);
    std::vector<uint8_t> label_data(num_images);

    images_in.read(reinterpret_cast<char*>(image_data.data()), image_data.size());
    labels_in.read(reinterpret_cast<char*>(label_data.data()), label_data.size());

    constexpr int num_classes = 10;
    ret.reserve(num_images);

    for (int i = 0; i < num_images; ++i) {
        Tensor<float, 3> image(1, rows, cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                image[0, r, c] = static_cast<float>(image_data[i * rows * cols + r * cols + c]);
            }
        }

        Vector<float> label = OneHot::type_to_onehot<num_classes>(label_data[i]);

        ret.emplace_back(std::move(image), std::move(label));
    }

    return ret;
}

class Layer {
    virtual void forward() = 0;
    virtual void backward() = 0;
    virtual ~Layer() = 0;
};

export class CNN {

};




/****************************** 使用示例 ******************************\
import modforge.deep_learning.cnn;


int main(int argc, char *argv[]) {

    auto dataset = load_dataset("./dataset/train-images.idx3-ubyte", "./dataset/train-labels.idx1-ubyte");


    return 0;
}
*/