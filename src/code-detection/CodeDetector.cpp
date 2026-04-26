//
// Created by Marco Stulic on 4/26/26.
//

#include "CodeDetector.hpp"

#include <chrono>
#include <print>
#include <imgui.h>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/objdetect.hpp>

#include "../Orchestrator.hpp"
#include "../io/FileIo.hpp"
#include "../io/CameraInput.hpp"

std::recursive_mutex CodeDetector::mutex;
std::thread* CodeDetector::processing_thread = nullptr;
std::atomic<bool> CodeDetector::should_run = false;
std::vector<Code> CodeDetector::current_codes;

void CodeDetector::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (processing_thread) {
        return;
    }

    should_run = true;
    processing_thread = new std::thread(main);
}

void CodeDetector::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (processing_thread) {
        should_run = false;
        processing_thread->join();
        delete processing_thread;
        processing_thread = nullptr;
    }
}

void CodeDetector::draw_debug_ui() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    ImGui::Text("Codes detected: %zu", current_codes.size());
    for (const auto& c : current_codes) {
        ImGui::Text("Code ID: %d at (%.2f, %.2f)", c.id, c.location[0], c.location[1]);
    }
}

void CodeDetector::main() {
    ryml::Tree config = aims::parse_config("general_config.yml");
    std::string config_cam = "default";
    if (config.rootref().has_child("camera_use") && config["camera_use"].has_child("code_detection")) {
        config["camera_use"]["code_detection"] >> config_cam;
    }

    auto cap = aims::get_camera(config_cam);

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_7X7_1000);
    cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
    cv::aruco::ArucoDetector detector(dictionary, detectorParams);

    while (should_run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        cv::Mat frame;
        if (!aims::read_camera(config_cam, frame) || frame.empty()) continue;

        cv::Mat debug_frame = frame.clone();

        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
        detector.detectMarkers(frame, markerCorners, markerIds, rejectedCandidates);

        std::vector<Code> found_codes;
        if (!markerIds.empty()) {
            cv::aruco::drawDetectedMarkers(debug_frame, markerCorners, markerIds);
            for (size_t i = 0; i < markerIds.size(); ++i) {
                Code c;
                c.id = markerIds[i];
                cv::Point2f center(0, 0);
                for (const auto& pt : markerCorners[i]) {
                    center.x += pt.x;
                    center.y += pt.y;
                }
                center.x /= 4.0f;
                center.y /= 4.0f;
                c.location = cv::Vec2f(center.x, center.y);
                found_codes.push_back(c);
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            current_codes = found_codes;
        }

        aims::orchestrator().set_codes(found_codes);

        auto debug_view = aims::orchestrator().get_view("Code Debug");
        if (debug_view) {
            debug_view->new_from_cv(debug_frame);
        }
    }
}
