//
// Created by user on 3/14/26.
//

#include "View.hpp"

#include <utility>

namespace aims
{
    View::View(std::string id) : view_id(std::move(id)), texture(aims_graphx::GpuTextureHandle{0}) {
    }

    View::~View() = default;

    void View::new_from_cv(const cv::Mat& mat) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        dirty_flag_gpu = true;
        rgb_texture = aims_graphx::mat_to_rgbtex(mat);
    }

    aims_graphx::GpuTextureHandle View::get_texture() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (!dirty_flag_gpu) {
            return texture;
        }

        texture = aims_graphx::upload_rgb(rgb_texture, view_id);
        return texture;
    }

    std::string View::get_id() const {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return view_id;
    }

}
