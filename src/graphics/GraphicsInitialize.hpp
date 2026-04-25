#pragma once
#include <SDL3/SDL_video.h>
#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <imgui/imgui.h>

#include "glad/gl.h"
#include <opencv4/opencv2/opencv.hpp>

namespace aims_graphx
{
    struct GraphicsContext
    {
        SDL_Window* window;
        SDL_GLContext gl_context;
        ImGuiIO& io;
    };

    struct GpuTextureHandle {
        GLuint texture;
        int width = 0;
        int height = 0;
    };

    struct TextureResolution {
        int width = 0;
        int height = 0;

        [[nodiscard]] float aspect_ratio() const {
            return (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
        }
    };

    struct RGBTex {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::vector<uint8_t> data;

        [[nodiscard]] float aspect_ratio() const {
            return (height > 0) ? (static_cast<float>(width) / height) : 1.0f;
        }
    };

    std::expected<GraphicsContext, std::string> initialize();
    void destroy(const std::shared_ptr<GraphicsContext>& context);

    void imgui_set_colors();

    GpuTextureHandle& upload_rgb(const RGBTex& texture, std::string key);
    [[nodiscard]] TextureResolution get_texture_resolution(const GpuTextureHandle& handle);
    RGBTex mat_to_rgbtex(const cv::Mat& mat);
}
