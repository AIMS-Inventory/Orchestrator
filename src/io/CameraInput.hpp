//
// Created by Marco Stulic on 3/17/26.
//

#pragma once
#include <mutex>

#include "../graphics/View.hpp"

namespace cv {
    class VideoCapture;
}

namespace aims {
    class CameraInput {
    public:
        CameraInput(std::shared_ptr<aims::View> view, std::string camera_id = "default");

        std::shared_ptr<aims::View> get_view();
        std::shared_ptr<cv::VideoCapture> get_camera();
        std::string get_camera_id() const;
    protected:
        mutable std::recursive_mutex mutex;

        std::string camera_id;
        std::shared_ptr<aims::View> view;
        std::shared_ptr<cv::VideoCapture> camera;
    };
}
