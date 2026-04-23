//
// Created by Marco Stulic on 4/23/26.
//

#include "FacialRecognition.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <print>

#include <opencv4/opencv2/opencv.hpp>

std::recursive_mutex FacialRecognition::mutex;
std::thread* FacialRecognition::processing_thread = nullptr;
std::atomic<bool> FacialRecognition::should_run = false;
std::vector<PersonEntry> FacialRecognition::known_people;

namespace {
    bool is_png(const std::filesystem::path& path) {
        return path.extension() == ".png";
    }

    cv::Mat to_bgr(const cv::Mat& source) {
        if (source.empty()) {
            return {};
        }

        if (source.channels() == 3) {
            return source;
        }

        cv::Mat converted;
        if (source.channels() == 4) {
            cv::cvtColor(source, converted, cv::COLOR_BGRA2BGR);
        } else if (source.channels() == 1) {
            cv::cvtColor(source, converted, cv::COLOR_GRAY2BGR);
        } else {
            source.convertTo(converted, CV_8U);
        }
        return converted;
    }

    cv::Mat extract_embedding(const cv::Mat& source, cv::dnn::Net& net) {
        cv::Mat face = to_bgr(source);
        if (face.empty()) {
            return {};
        }

        cv::resize(face, face, cv::Size(112, 112));
        face.convertTo(face, CV_32F, 1.0 / 255.0);

        if (!net.empty()) {
            const cv::Mat blob = cv::dnn::blobFromImage(face, 1.0, cv::Size(112, 112), cv::Scalar(), true, false, CV_32F);
            net.setInput(blob);

            cv::Mat embedding = net.forward();
            if (!embedding.empty()) {
                embedding = embedding.reshape(1, 1).clone();
                cv::normalize(embedding, embedding);
                return embedding;
            }
        }

        cv::Mat fallback = face.reshape(1, 1).clone();
        cv::normalize(fallback, fallback);
        return fallback;
    }

    std::vector<std::filesystem::path> list_people_images(const std::filesystem::path& people_dir) {
        std::vector<std::filesystem::path> image_paths;

        for (const auto& entry : std::filesystem::directory_iterator(people_dir)) {
            if (entry.is_regular_file() && is_png(entry.path())) {
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
    const std::filesystem::path face_model_path = std::filesystem::path("resources") / "models" / "sface.onnx";

    cv::dnn::Net embedding_net;
    if (std::filesystem::exists(face_model_path)) {
        try {
            embedding_net = cv::dnn::readNetFromONNX(face_model_path.string());
        } catch (const cv::Exception& error) {
            std::print("Warning: Could not load face embedding model '{}': {}\n", face_model_path.string(), error.what());
        }
    } else {
        std::print("Warning: Face embedding model '{}' was not found. Falling back to resized image embeddings.\n", face_model_path.string());
    }

    if (std::filesystem::exists(people_dir) && std::filesystem::is_directory(people_dir)) {
        const auto image_paths = list_people_images(people_dir);
        for (std::size_t index = 0; index < image_paths.size(); ++index) {
            const auto& image_path = image_paths[index];
            std::print("Loading person image '{}'\n", image_path.string());

            cv::Mat image = cv::imread(image_path.string(), cv::IMREAD_UNCHANGED);
            if (image.empty()) {
                std::print("Warning: Could not read image '{}'\n", image_path.string());
                continue;
            }

            cv::Mat face_encoding = extract_embedding(image, embedding_net);
            if (face_encoding.empty()) {
                std::print("Warning: Could not extract an embedding for '{}'\n", image_path.string());
                continue;
            }

            known_people.push_back(PersonEntry{image_path.stem().string(), static_cast<int>(index), face_encoding});
        }
    } else {
        std::print("Warning: People directory '{}' does not exist.\n", people_dir.string());
    }

    should_run = true;
    processing_thread = new std::thread(main);
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
    while (should_run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
