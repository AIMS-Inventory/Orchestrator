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
        texture = aims_graphx::upload_rgb(aims_graphx::mat_to_rgbtex(mat), view_id+"_vt"); // short for viewtex
    }

    aims_graphx::GpuTextureHandle View::get_texture() const {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return texture;
    }

    std::string View::get_id() const {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return view_id;
    }

}
