//
// Created by user on 3/14/26.
//

#pragma once

#include <mutex>

#include "GraphicsInitialize.hpp"

namespace aims
{
    class View {
    public:
        explicit View(std::string id);
        ~View();

        void new_from_cv(const cv::Mat& mat);
        aims_graphx::GpuTextureHandle get_texture();
        std::string get_id() const;

    protected:
        std::string view_id;
        aims_graphx::RGBTex rgb_texture;
        aims_graphx::GpuTextureHandle texture;
        bool dirty_flag_gpu = false;
        mutable std::recursive_mutex mutex;
    };
}
