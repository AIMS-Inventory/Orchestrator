//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <vector>
#include <map>
#include <string>
#include <opencv2/dnn.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

#include "Pill.hpp"

class PillRecognition {
public:
    static void initialize();
    static void shutdown();
    static void draw_debug_ui();

    static std::vector<Pill> recognize_pills();

protected:
    static cv::dnn::Net net;
    static cv::Ptr<cv::ORB> orb;
    static cv::BFMatcher matcher;
    static std::map<std::string, cv::Mat> pill_ref_images;
    static int get_match_score(const cv::Mat& descriptors_scene, const std::string& pill_name);

    static std::vector<Pill> last_detected_pills;
};
