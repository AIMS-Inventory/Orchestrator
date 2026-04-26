//
// Created by Marco Stulic on 4/23/26.
//

#pragma once
#include <opencv2/core/mat.hpp>
#include <string>

struct PersonEntry {
    std::string name;
    int id;
    cv::Mat face_encoding;
    double last_seen;
    double confidence;
    std::string extra_info;
};
