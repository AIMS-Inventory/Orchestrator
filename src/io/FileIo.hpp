//
// Created by Marco Stulic on 3/16/26.
//

#pragma once
#include <string>
#include <fstream>
#include <ryml.hpp>
#include <vector>

namespace cv {
    class VideoCapture;
}

namespace aims {
    std::vector<char> open_config_file(const std::string &path);
    std::filebuf get_filepath(const std::string &sub_path);
    void load_cameras();
    ryml::Tree parse_config(const std::string &path);
    std::shared_ptr<cv::VideoCapture> get_camera(std::string camera_name);
    std::vector<std::string> enumerate_cams();
    std::vector<std::filesystem::path> enumerate_scripts();
}
