//
// Created by Marco Stulic on 4/26/26.
//

#include <string>
#include <vector>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>

struct Pill {
    std::string name;
    int quantity;
    cv::Rect bounds;
    float confidence;
};
