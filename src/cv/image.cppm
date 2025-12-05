/*******************************************************************************
 * @Author : hexne
 * @Data   : 2024/10/16 22:09
*******************************************************************************/

module;
#include <opencv2/opencv.hpp>
export module modforge.image;

export class Image {
    cv::Mat image;
public:
    Image() = default;
    ~Image() = default;
};
