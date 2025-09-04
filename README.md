cnn模块使用样例
```
#include <iostream>
#include <vector>
#include <functional>
import modforge.progress;
import modforge.deep_learning.tools;
import modforge.console;
import modforge.deep_learning.cnn;
import modforge.tensor;
import modforge.time;

void load() {
    Time begin;
    CNN cnn;
    cnn.load("./module");

    cnn.load_dataset_func = load_mnist_dataset;
    cnn.test("./dataset/t10k-images.idx3-ubyte", "./dataset/t10k-labels.idx1-ubyte");
    std::cout << "测试耗时："  << (Time::now() - begin).get_clock_string() << std::endl;

}

void save() {

    CNN cnn(28, 28, 1);
    cnn.add_conv(8, 3);
    cnn.add_relu();
    cnn.add_pool( 2, 2 );

    cnn.add_conv(16, 3);
    cnn.add_relu( );
    cnn.add_pool( 2, 2 );

    cnn.add_conv(32, 3);
    cnn.add_relu( );

    cnn.add_fc(10 );

    Time begin;
    cnn.load_dataset_func = load_mnist_dataset;

    cnn.train("./dataset/train-images.idx3-ubyte", "./dataset/train-labels.idx1-ubyte", 10);
    std::cout << "训练耗时："  << (Time::now() - begin).get_clock_string() << std::endl;
    cnn.save("./module");


}

int main() {

    save();
    load();

    return 0;
}


```
