/********************************************************************************
* @Author : hexne
* @Date   : 2025/12/10 17:32:23
********************************************************************************/

import modforge.image;
import std;


bool test_equal(const Image &img1, const Image &img2) {
    auto [c, w, h] = img1.extent();
    auto [c2, w2, h2] = img2.extent();
    if (c != c2 or w != w2 or h != h2)
        return false;

    for (int z = 0; z < c; ++z) {
        for (int i = 0; i < w; ++i) {
            for (int j = 0; j < h; ++j) {
                if (img1[z, i, j] != img2[z, i, j])
                    return false;
            }
        }
    }
    return true;
}

int test_image() {
    Image image("image.jpg");
    if (image.channels() != 3)
        return __LINE__;

    if (auto [w, h] = image.size(); w != 1920 or h != 1080)
        return __LINE__;

    if (auto [c, w, h] = image.extent(); c != 3 or w != 1920 or h != 1080)
        return __LINE__;

    // 浅拷贝
    Image image2 = image;
    image2[0,0,0] = 0;
    if (!test_equal(image, image2))
        return __LINE__;

    Image image3(image2);
    image3[0,0,0] = 1;
    if (!test_equal(image2, image3))
        return __LINE__;

    // 移动构造和拷贝也应是浅拷贝
    Image image4 = std::move(image3);
    image4[0,0,0] = 2;
    if (!test_equal(image3, image4))
        return __LINE__;

    Image image5(std::move(image4));
    image5[0,0,0] = 3;
    if (!test_equal(image4, image5))
        return __LINE__;

    // 深拷贝
    Image image6 = image5.clone();
    image6[0,0,0] = 4;
    // 此处不应相同
    if (test_equal(image5, image6))
        return __LINE__;

    Image image7(image6.clone());
    image7[0,0,0] = 5;
    if (test_equal(image6, image7))
        return __LINE__;

    // 测试旋转
    image.rotate(180);
    image.rotate(180);
        // tmp.show_and_wait("show", 10);

    // 测试缩放
    image.zoom(0.5);
    image.show_and_wait("show", 10);

    image6.width(512);
    image6.height(256);
    image6.show_and_wait("show", 10);

    auto data = image6.get_histogram_data();
    image6.to_grayscale();
    data = image6.get_histogram_data();
    auto tmp = image6.clone();
    tmp.to_binary(100);
    tmp.show_and_wait("show", 1);



    return 0;
}