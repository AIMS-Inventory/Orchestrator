//
// Created by Marco Stulic on 3/17/26.
//

#include "CameraInput.hpp"

#include "FileIo.hpp"

aims::CameraInput::CameraInput(std::shared_ptr<aims::View> view, std::string camera_id) : camera_id(std::move(camera_id)), view(std::move(view)) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    camera = aims::get_camera(this->camera_id);
}

std::shared_ptr<aims::View> aims::CameraInput::get_view() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return view;
}

std::shared_ptr<cv::VideoCapture> aims::CameraInput::get_camera() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return camera;
}

std::string aims::CameraInput::get_camera_id() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return camera_id;
}