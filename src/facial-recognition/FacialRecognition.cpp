//
// Created by Marco Stulic on 4/23/26.
//

#include "FacialRecognition.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <print>

#include <opencv4/opencv2/opencv.hpp>

#include "../io/FileIo.hpp"
#include "../io/CameraInput.hpp"
#include "../Orchestrator.hpp"
#include <imgui.h>

std::recursive_mutex FacialRecognition::mutex;
std::thread* FacialRecognition::processing_thread = nullptr;
std::atomic<bool> FacialRecognition::should_run = false;
std::vector<PersonEntry> FacialRecognition::known_people;
std::vector<PersonEntry> FacialRecognition::active_people;

namespace {
    bool is_image(const std::filesystem::path& path) {
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
    }

    std::vector<std::filesystem::path> list_people_images(const std::filesystem::path& people_dir) {
        std::vector<std::filesystem::path> image_paths;

        for (const auto& entry : std::filesystem::directory_iterator(people_dir)) {
            if (entry.is_regular_file() && is_image(entry.path())) {
                image_paths.push_back(entry.path());
            }
        }

        std::sort(image_paths.begin(), image_paths.end());
        return image_paths;
    }
}


void FacialRecognition::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (processing_thread) {
        return; // already initialized
    }

    known_people.clear();

    const std::filesystem::path people_dir = std::filesystem::path("resources") / "people";

    ryml::Tree config = aims::parse_config("general_config.yml");

    std::string face_detect_model = "resources/models/yunet.onnx";
    if (config.rootref().has_child("models") && config["models"].has_child("face_detection")) {
        config["models"]["face_detection"] >> face_detect_model;
    }

    if (face_detect_model.starts_with("../")) {
        face_detect_model = face_detect_model.substr(3);
    }

    std::string face_rec_model = "resources/models/sface.onnx";
    if (config.rootref().has_child("models") && config["models"].has_child("face_recognition")) {
        config["models"]["face_recognition"] >> face_rec_model;
    }

    if (face_rec_model.starts_with("../")) {
        face_rec_model = face_rec_model.substr(3);
    }

    cv::Ptr<cv::FaceDetectorYN> face_detector;
    try {
        face_detector = cv::FaceDetectorYN::create(face_detect_model, "", cv::Size(320, 320), 0.6f, 0.3f, 5000);
    } catch (const cv::Exception& e) {
        std::print("Warning: Could not load face detection model '{}': {}\n", face_detect_model, e.what());
    }

    cv::Ptr<cv::FaceRecognizerSF> face_recognizer;
    try {
        face_recognizer = cv::FaceRecognizerSF::create(face_rec_model, "");
    } catch (const cv::Exception& e) {
        std::print("Warning: Could not load face recognition model '{}': {}\n", face_rec_model, e.what());
    }

    if (std::filesystem::exists(people_dir) && std::filesystem::is_directory(people_dir)) {
        const auto image_paths = list_people_images(people_dir);
        for (std::size_t index = 0; index < image_paths.size(); ++index) {
            const auto& image_path = image_paths[index];
            std::print("Loading person image '{}'\n", image_path.string());

            cv::Mat image = cv::imread(image_path.string(), cv::IMREAD_COLOR);
            if (image.empty()) {
                std::print("Warning: Could not read image '{}'\n", image_path.string());
                continue;
            }

            cv::Mat face_encoding;
            if (face_detector && face_recognizer) {
                face_detector->setInputSize(image.size());
                cv::Mat faces;
                face_detector->detect(image, faces);
                if (faces.rows > 0) {
                    cv::Mat aligned_img;
                    face_recognizer->alignCrop(image, faces.row(0), aligned_img);
                    face_recognizer->feature(aligned_img, face_encoding);
                    face_encoding = face_encoding.clone();
                }
            }

            if (face_encoding.empty()) {
                std::print("Warning: Could not extract an embedding for '{}'\n", image_path.string());
                continue;
            }

            known_people.push_back(PersonEntry{image_path.stem().string(), static_cast<int>(index), face_encoding, 0.0, 1.0, ""});
        }
    } else {
        std::print("Warning: People directory '{}' does not exist.\n", people_dir.string());
    }

    should_run = true;
    processing_thread = new std::thread(main);
}

std::vector<PersonEntry> FacialRecognition::get_people_on_screen() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return active_people;
}

std::vector<PersonEntry> FacialRecognition::get_all_known_people() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return known_people;
}

void FacialRecognition::draw_debug_ui() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    ImGui::Text("Faces detected: %zu", active_people.size());
    for (const auto& p : active_people) {
        ImGui::Text("Person: %s (Conf: %.2f)", p.name.c_str(), p.confidence);
    }
}

void FacialRecognition::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (processing_thread) {
        should_run = false;
        processing_thread->join();
        delete processing_thread;
        processing_thread = nullptr;
    }
}

void FacialRecognition::main() {
    // Load config
    ryml::Tree config = aims::parse_config("general_config.yml");
    std::string config_cam = "default";
    if (config.rootref().has_child("camera_use") && config["camera_use"].has_child("facial_recognition")) {
        config["camera_use"]["facial_recognition"] >> config_cam;
    }

    std::string face_detect_model = "extern/yunet.onnx";
    if (config.rootref().has_child("models") && config["models"].has_child("face_detection")) {
        config["models"]["face_detection"] >> face_detect_model;
    }

    if (face_detect_model.starts_with("../")) {
        face_detect_model = face_detect_model.substr(3);
    }

    std::string face_rec_model = "extern/sface.onnx";
    if (config.rootref().has_child("models") && config["models"].has_child("face_recognition")) {
        config["models"]["face_recognition"] >> face_rec_model;
    }

    if (face_rec_model.starts_with("../")) {
        face_rec_model = face_rec_model.substr(3);
    }

    cv::Ptr<cv::FaceDetectorYN> face_detector;
    try {
        face_detector = cv::FaceDetectorYN::create(face_detect_model, "", cv::Size(320, 320), 0.6f, 0.3f, 5000);
    } catch (const cv::Exception& e) {
        std::print("Warning: Could not load face detection model '{}': {}\n", face_detect_model, e.what());
    }

    cv::Ptr<cv::FaceRecognizerSF> face_recognizer;
    try {
        face_recognizer = cv::FaceRecognizerSF::create(face_rec_model, "");
    } catch (const cv::Exception& e) {
        std::print("Warning: Could not load face recognition model '{}': {}\n", face_rec_model, e.what());
    }

    auto cap = aims::get_camera(config_cam);

    while (should_run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!cap || !cap->isOpened() || !face_detector || !face_recognizer)
            continue;

        cv::Mat frame;
        cap->read(frame);
        if (frame.empty()) continue;

        cv::Mat debug_frame = frame.clone();

        face_detector->setInputSize(frame.size());
        cv::Mat faces;
        face_detector->detect(frame, faces);

        std::vector<PersonEntry> current_frame_people;

        for (int i = 0; i < faces.rows; i++) {
            cv::Mat face_aligned, feature;
            face_recognizer->alignCrop(frame, faces.row(i), face_aligned);
            face_recognizer->feature(face_aligned, feature);

            std::string best_name = "Unknown";
            double max_sim = 0.0;

            const double threshold = 0.45;
            for (const auto& kp : known_people) {
                double sim = face_recognizer->match(feature, kp.face_encoding, cv::FaceRecognizerSF::FR_COSINE);
                if (sim > max_sim) {
                    max_sim = sim;
                    if (sim > threshold) {
                        best_name = kp.name;
                    }
                }
            }

            float x = faces.at<float>(i, 0);
            float y = faces.at<float>(i, 1);
            float w = faces.at<float>(i, 2);
            float h = faces.at<float>(i, 3);

            int ix = std::max(0, std::min((int)std::round(x), frame.cols - 1));
            int iy = std::max(0, std::min((int)std::round(y), frame.rows - 1));
            int iw = std::max(0, std::min((int)std::round(w), frame.cols - ix));
            int ih = std::max(0, std::min((int)std::round(h), frame.rows - iy));

            cv::Rect box(ix, iy, iw, ih);
            cv::rectangle(debug_frame, box, cv::Scalar(0, 255, 0), 2);
            std::string label = best_name + " (" + std::to_string(max_sim).substr(0,4) + ")";
            cv::putText(debug_frame, label, cv::Point(box.x, std::max(0, box.y - 10)), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0, 255, 0), 2);

            PersonEntry p_entry;
            p_entry.name = best_name;
            p_entry.confidence = max_sim;
            p_entry.last_seen = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            current_frame_people.push_back(p_entry);
        }

        {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            active_people = current_frame_people;
        }

        auto debug_view = aims::orchestrator().get_view("Face Debug");
        if (debug_view) {
            debug_view->new_from_cv(debug_frame);
        }
    }
}
