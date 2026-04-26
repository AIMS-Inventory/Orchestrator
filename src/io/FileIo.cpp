//
// Created by Marco Stulic on 3/16/26.
//

#include "FileIo.hpp"
#include <unordered_map>
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <print>
#include <mutex>
#include <opencv2/opencv.hpp>

#include "../graphics/View.hpp"


namespace aims {
    static std::unordered_map<std::string, std::shared_ptr<cv::VideoCapture>> cameras;
    static std::recursive_mutex camera_mutex;
    static std::filesystem::path app_path = std::filesystem::current_path();
    static std::filesystem::path DEFAULT_CONFIG_PATH = "resources/configs";
    static std::filesystem::path SCRIPTS_PATH = "resources/scripts";

    std::vector<char> open_config_file(const std::string &path) {
        std::vector<char> data;
        std::filesystem::path config_path = app_path / DEFAULT_CONFIG_PATH / path;

        std::ifstream file(config_path, std::ios::binary);
        if (!file) {
            std::print("Failed to open config file at path: {}\n", config_path.string());
            return data;
        }

        int file_size = 0;
        file.seekg(std::ios::end);
        file_size = file.tellg();
        file.seekg(std::ios::beg);

        data.reserve(file_size);
        data.insert(data.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        return data;
    }

    ryml::Tree parse_config(const std::string &path) {
        auto config_vals = open_config_file(path);
        const ryml::csubstr config_str(config_vals.data(), config_vals.size());
        return ryml::parse_in_arena(config_str);
    }

    void load_cameras() {
        auto data = open_config_file("camera_indices.yml");
        if (data.empty()) return;

        ryml::Tree tree = ryml::parse_in_arena(ryml::csubstr(data.data(), data.size()));
        auto root = tree.rootref();

        if (root.has_child("cams")) {
            for (auto const& cam_node : root["cams"].children()) {
                std::string cam_name;
                int cam_index = -1;

                if (cam_node.has_child("name")) {
                    cam_node["name"] >> cam_name;
                }
                if (cam_node.has_child("index")) {
                    cam_node["index"] >> cam_index;
                }

                if (!cam_name.empty() && cam_index != -1) {
                    cameras.emplace(cam_name, std::make_shared<cv::VideoCapture>(cam_index));

                    if (!cameras[cam_name]->isOpened()) {
                        std::print("Warning: Could not open camera {} at index {}\n", cam_name, cam_index);
                    }
                }
            }
        }
    }

    std::shared_ptr<cv::VideoCapture> get_camera(std::string camera_name) {
        std::lock_guard<std::recursive_mutex> lock(camera_mutex);
        auto it = cameras.find(camera_name);
        if (it != cameras.end()) {
            return it->second;
        } else {
            std::print("Camera with name '{}' not found in configuration.\n", camera_name);
            return nullptr;
        }
    }

    bool read_camera(const std::string& camera_name, cv::Mat& out_frame) {
        std::lock_guard<std::recursive_mutex> lock(camera_mutex);
        auto cap = get_camera(camera_name);
        if (!cap || !cap->isOpened()) {
            return false;
        }
        return cap->read(out_frame);
    }

    std::vector<std::string> enumerate_cams() {
        std::lock_guard<std::recursive_mutex> lock(camera_mutex);
        std::vector<std::string> cam_list;
        for (const auto& [name, cap] : cameras) {
            if (cap && cap->isOpened()) {
                cam_list.emplace_back(name);
            }
        }
        return cam_list;
    }

    std::vector<std::filesystem::path> enumerate_scripts() {
        std::vector<std::filesystem::path> script_paths;
        std::filesystem::path scripts_dir = app_path / SCRIPTS_PATH;

        if (std::filesystem::exists(scripts_dir) && std::filesystem::is_directory(scripts_dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(scripts_dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".py") {
                    script_paths.push_back(entry.path());
                } else {
                    std::print("Not a python file '{}'\n", entry.path().string());
                }
            }
        } else {
            std::print("Scripts directory '{}' does not exist.\n", scripts_dir.string());
        }

        return script_paths;
    }

    std::filebuf get_filepath(const std::string &sub_path) {
        std::filesystem::path full_path = app_path / sub_path;
        std::filebuf file_buffer;
        file_buffer.open(full_path, std::ios::in | std::ios::binary);
        if (!file_buffer.is_open()) {
            std::print("Failed to open file at path: {}\n", full_path.string());
        }
        return file_buffer;
    }

}
