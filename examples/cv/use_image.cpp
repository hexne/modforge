/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 20:19:51
********************************************************************************/
import std;
import modforge.image;

void use_image() {

    // 实现了默认构造函数，拷贝构造函数，移动构造函数以及相应的赋值运算符
    Image image;
    image.open("image.jpg");
    auto is_open = image.is_open();

    Image image2("image.jpg");

    // 尺寸缩放
    auto width = image.width();
    image.width(width);
    auto height = image.height();
    image.height(height);
    auto [w, h] = image.size();
    image.size(w, h);
    auto [z, w2, h2] = image.extent();

    // 逆时针旋转180°
    image.rotate(180);

    // 缩放1.5倍
    image.zoom(1.5);

    // 功能
    // 转灰度图
    image.to_grayscale();

    // 根据阈值转二值图,二值图的值为0/255
    image.to_binary(100);

    // 获取每个通道每个像素值统计次数
    auto data = image.get_histogram_data();

}