//
// Created by Marco Stulic on 3/16/26.
//

#include "FileIo.hpp"
#include <unordered_map>
#include <ryml.hpp>
#include <ryml_std.hpp>
#include <print>

#include "../graphics/View.hpp"


namespace aims {
    static std::unordered_map<std::string, std::shared_ptr<cv::VideoCapture>> cameras;
    static std::filesystem::path app_path = std::filesystem::current_path();
    static std::filesystem::path DEFAULT_CONFIG_PATH = "resources/configs";

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
        const ryml::substr config_str(config_vals.data(), config_vals.size());
        return ryml::parse_in_place(config_str);
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
        auto it = cameras.find(camera_name);
        if (it != cameras.end()) {
            return it->second;
        } else {
            std::print("Camera with name '{}' not found in configuration.\n", camera_name);
            return nullptr;
        }
    }

    std::vector<std::string> enumerate_cams() {
        std::vector<std::string> cam_list;
        for (const auto& [name, cap] : cameras) {
            if (cap && cap->isOpened()) {
                cam_list.emplace_back(name);
            }
        }
        return cam_list;
    }

}
