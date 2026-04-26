//
// Created by Marco Stulic on 4/26/26.
//

#include "PillRecognition.hpp"

#include <iostream>
#include <filesystem>
#include <imgui.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include "../Orchestrator.hpp"
#include "../io/FileIo.hpp"
#include "../io/CameraInput.hpp"

namespace py = pybind11;

cv::dnn::Net PillRecognition::net;
cv::Ptr<cv::ORB> PillRecognition::orb;
cv::BFMatcher PillRecognition::matcher;
std::map<std::string, cv::Mat> PillRecognition::pill_ref_images;
std::vector<Pill> PillRecognition::last_detected_pills;

constexpr double PILL_DETECTOR_THRESHOLD = 0.8;
constexpr double PILL_NMS_THRESHOLD = 0.8;
const int INPUT_H = 480;
const int INPUT_W = 832;

void PillRecognition::initialize() {
    ryml::Tree config = aims::parse_config("general_config.yml");
    std::string model_path = "";
    if (config.rootref().has_child("models") && config["models"].has_child("pill_detector")) {
        config["models"]["pill_detector"] >> model_path;
    }

    if (model_path.empty()) {
        std::cerr << "PillRecognition: No model path specified!" << std::endl;
        return;
    }

    try {
        net = cv::dnn::readNet(model_path);
        std::cout << "PillRecognition: Successfully loaded model from " << model_path << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "PillRecognition: Failed to load model: " << e.what() << std::endl;
    }

    orb = cv::ORB::create(1000);
    matcher = cv::BFMatcher(cv::NORM_HAMMING, true);

    std::string dir = "";
    if (config.rootref().has_child("models") && config["models"].has_child("pill_images_dir")) {
        config["models"]["pill_images_dir"] >> dir;
    }

    if (!dir.empty()) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                const std::string path = entry.path().string();
                const std::string filename = entry.path().stem().string();
                cv::Mat img = cv::imread(path);
                if (!img.empty()) {
                    pill_ref_images[filename] = img;
                }
            }
        }
    }
}

void PillRecognition::shutdown() {
}

int PillRecognition::get_match_score(const cv::Mat& descriptors_scene, const std::string& pill_name) {
    if (pill_ref_images.find(pill_name) == pill_ref_images.end()) return 0;
    cv::Mat& img_object = pill_ref_images[pill_name];
    if (img_object.empty()) return 0;

    std::vector<cv::KeyPoint> keypoints_object;
    cv::Mat descriptors_object;
    if (img_object.cols >= 32 && img_object.rows >= 32) {
        orb->detectAndCompute(img_object, cv::noArray(), keypoints_object, descriptors_object);
    }
    if (descriptors_object.empty() || descriptors_scene.empty()) return 0;

    std::vector<cv::DMatch> matches;
    matcher.match(descriptors_object, descriptors_scene, matches);
    return matches.size();
}

std::vector<Pill> PillRecognition::recognize_pills() {
    ryml::Tree config = aims::parse_config("general_config.yml");
    std::string config_cam = "default";
    if (config.rootref().has_child("camera_use") && config["camera_use"].has_child("pill_detection_cam")) {
        config["camera_use"]["pill_detection_cam"] >> config_cam;
    }

    cv::Mat frame;
    if (!aims::read_camera(config_cam, frame) || frame.empty()) {
        return {};
    }

    if (net.empty()) {
        return {};
    }

    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0 / 255.0, cv::Size(INPUT_W, INPUT_H), cv::Scalar(), true, false, CV_32F);
    net.setInput(blob);
    cv::Mat raw = net.forward();

    cv::Mat detections;
    int num_features;
    if (raw.dims == 3 && raw.size[1] < raw.size[2]) {
        num_features = raw.size[1];
        cv::Mat temp = raw.reshape(1, num_features);
        cv::transpose(temp, detections);
    } else {
        num_features = raw.size[raw.dims - 1];
        detections = raw.reshape(1, static_cast<int>(raw.total() / num_features));
    }

    std::vector<cv::Rect> raw_boxes;
    std::vector<float> confidences;

    for (int i = 0; i < detections.rows; ++i) {
        const float* det = detections.ptr<float>(i);
        const float conf = (num_features >= 6) ? det[4] * det[5] : det[4];
        if (conf < PILL_DETECTOR_THRESHOLD) continue;

        float cx = det[0];
        float cy = det[1];
        float w  = det[2];
        float h  = det[3];

        int width  = static_cast<int>(w / INPUT_W * frame.cols);
        int height = static_cast<int>(h / INPUT_H * frame.rows);
        int x      = static_cast<int>((cx / INPUT_W * frame.cols) - (width / 2));
        int y      = static_cast<int>((cy / INPUT_H * frame.rows) - (height / 2));

        cv::Rect unclamped_box(x, y, width, height);
        cv::Rect frame_bounds(0, 0, frame.cols, frame.rows);
        cv::Rect box = unclamped_box & frame_bounds;

        if (box.width > 0 && box.height > 0) {
            raw_boxes.push_back(box);
            confidences.push_back(conf);
        }
    }

    std::vector<int> nms_indices;
    cv::dnn::NMSBoxes(raw_boxes, confidences, PILL_DETECTOR_THRESHOLD, PILL_NMS_THRESHOLD, nms_indices);

    cv::Mat annotated = frame.clone();
    std::vector<Pill> results;

    for (int idx : nms_indices) {
        cv::Rect box = raw_boxes[idx];
        float conf = confidences[idx];

        cv::rectangle(annotated, box, cv::Scalar(0, 255, 0), 2);

        cv::Mat pill_roi = frame(box).clone();
        cv::Mat gray_roi;
        cv::cvtColor(pill_roi, gray_roi, cv::COLOR_BGR2GRAY);

        std::vector<cv::KeyPoint> kp_scene;
        cv::Mat descriptors_scene;
        if (gray_roi.cols >= 32 && gray_roi.rows >= 32) {
            orb->detectAndCompute(gray_roi, cv::noArray(), kp_scene, descriptors_scene);
        }

        std::string best_pill_name = "misc";
        int best_score = 0;
        if (!descriptors_scene.empty()) {
            for (const auto& [pill_name, ref_img] : pill_ref_images) {
                int score = get_match_score(descriptors_scene, pill_name);
                if (score > best_score) {
                    best_score = score;
                    best_pill_name = pill_name;
                }
            }
        }

        // Add to results
        auto exists = std::find_if(results.begin(), results.end(), [&best_pill_name](const Pill& p) {
            return p.name == best_pill_name;
        });

        if (exists == results.end()) {
            Pill p;
            p.name = best_pill_name;
            p.quantity = 1;
            p.bounds = box;
            p.confidence = conf;
            results.push_back(p);
        } else {
            exists->quantity++;
            exists->bounds = box; // updating bounds to last found
            exists->confidence = conf;
        }

        std::string label = best_pill_name + " " + cv::format("%.2f", conf);
        cv::putText(annotated, label, cv::Point(box.x, box.y - 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    }

    last_detected_pills = results;

    auto debug_view = aims::orchestrator().get_view("Pill Debug");
    if (debug_view) {
        debug_view->new_from_cv(annotated);
    } else {
        auto view = std::make_shared<aims::View>("Pill Debug");
        view->new_from_cv(annotated);
        aims::orchestrator().push_view(view);
    }

    return results;
}

void PillRecognition::draw_debug_ui() {
    if (ImGui::Button("Recognize Pills")) {
        recognize_pills();
    }

    ImGui::Text("Last Detected Pills:");
    for (const auto& p : last_detected_pills) {
        ImGui::Text("Pill: %s, Qty: %d, Conf: %.2f (x:%d, y:%d, w:%d, h:%d)",
            p.name.c_str(), p.quantity, p.confidence, p.bounds.x, p.bounds.y, p.bounds.width, p.bounds.height);
    }
}

PYBIND11_EMBEDDED_MODULE(aims_pill, m) {
    py::class_<cv::Rect>(m, "Rect")
        .def_readwrite("x", &cv::Rect::x)
        .def_readwrite("y", &cv::Rect::y)
        .def_readwrite("width", &cv::Rect::width)
        .def_readwrite("height", &cv::Rect::height);

    py::class_<Pill>(m, "Pill")
        .def_readwrite("name", &Pill::name)
        .def_readwrite("quantity", &Pill::quantity)
        .def_readwrite("bounds", &Pill::bounds)
        .def_readwrite("confidence", &Pill::confidence);

    m.def("recognize_pills", &PillRecognition::recognize_pills, "Trigger pill recognition and get results");
}
