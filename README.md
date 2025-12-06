# 模块
## core
|      类       | 功能                       |
|:------------:|:-------------------------|
|  ArgsParser  | 参数解析                     |
| AverageQueue | 平均值队列                    |
|   Command    | 指令命令行并获取stdout输出         |
|   Console    | 终端控制，获取终端框高、以及控制光标的显示和隐藏 |
|    Cursor    | 控制光标位置和按键点击              |
| ProgressBar  | 进度条                      |
|    Tensor    | 张量运算                     |
|     Time     | 时间                       |
|    Timer     | 计时器                      |
|    Window    | 窗口                       |

## cv
|   类    | 功能      |
|:------:|:--------|
| Image  | 图片处理和显示 |
| Video  | 视频加载和显示 |
| Camera | 摄像头操作   |

## deep_learning
|  类  | 功能      |
|:---:|:--------|
| BP  | BP神经网络  |
| CNN | CNN卷积网络 |










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
