//
// Created by user on 3/14/26.
//

#include "GraphicsInitialize.hpp"

#include <expected>
#include <string>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <glad/gl.h>

#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include <opencv4/opencv2/opencv.hpp>

namespace aims_graphx
{
    static std::unordered_map<std::string, GpuTextureHandle> texture_cache;

    std::expected<GraphicsContext, std::string> initialize() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            return std::unexpected("Failed to initialize SDL: " + std::string(SDL_GetError()));
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        SDL_Window* window = SDL_CreateWindow("Aims Orchestrator Inspector", 1260, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (!window) {
            return std::unexpected("Failed to create window: " + std::string(SDL_GetError()));
        }

        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        if (!gl_context) {
            SDL_DestroyWindow(window);
            return std::unexpected("Failed to create OpenGL context: " + std::string(SDL_GetError()));
        }

        if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
            SDL_GL_DestroyContext(gl_context);
            SDL_DestroyWindow(window);
            return std::unexpected("Failed to initialize GLAD");
        }

        ImGui::SetCurrentContext(ImGui::CreateContext());

        if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context)) {
            SDL_GL_DestroyContext(gl_context);
            SDL_DestroyWindow(window);
            return std::unexpected("Failed to initialize ImGui SDL3 backend");
        }

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();

        ImGui_ImplOpenGL3_Init("#version 330");\

        imgui_set_colors();

        return GraphicsContext{window, gl_context, io};
    }

    void destroy(const std::shared_ptr<GraphicsContext>& context) {
        SDL_GL_DestroyContext(context->gl_context);
        SDL_DestroyWindow(context->window);
        SDL_Quit();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    GpuTextureHandle& upload_rgb(const RGBTex& texture, std::string key) {
        auto it = texture_cache.find(key);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (it != texture_cache.end()) {
            glBindTexture(GL_TEXTURE_2D, it->second.texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width, texture.height, GL_RGB, GL_UNSIGNED_BYTE, texture.data.data());
            return it->second;
        }

        GLuint gl_texture;
        glGenTextures(1, &gl_texture);
        glBindTexture(GL_TEXTURE_2D, gl_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data.data());

        GpuTextureHandle handle{gl_texture};
        texture_cache[key] = handle;
        return texture_cache[key];
    }

    RGBTex mat_to_rgbtex(const cv::Mat& mat) {
        aims_graphx::RGBTex tex;
        tex.width = mat.cols;
        tex.height = mat.rows;
        tex.channels = 3;

        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

        if (rgb.isContinuous()) {
            tex.data.assign(rgb.data, rgb.data + rgb.total() * rgb.channels());
        } else {
            for (int i = 0; i < rgb.rows; ++i) {
                tex.data.insert(tex.data.end(), rgb.ptr<uint8_t>(i), rgb.ptr<uint8_t>(i) + rgb.cols * 3);
            }
        }
        return tex;
    }

    // credit: https://github.com/ocornut/imgui/issues/707 @simv0loffical
    void imgui_set_colors() {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        const ImVec4 base_bg = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
        const ImVec4 base_bg_alt = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
        const ImVec4 elevated_bg = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
        const ImVec4 elevated_bg_hovered = ImVec4(0.16f, 0.19f, 0.23f, 1.00f);
        const ImVec4 elevated_bg_active = ImVec4(0.18f, 0.22f, 0.27f, 1.00f);
        const ImVec4 accent = ImVec4(0.37f, 0.61f, 0.92f, 1.00f);
        const ImVec4 accent_hovered = ImVec4(0.46f, 0.68f, 0.97f, 1.00f);
        const ImVec4 accent_active = ImVec4(0.31f, 0.54f, 0.85f, 1.00f);
        const ImVec4 border = ImVec4(0.11f, 0.20f, 0.34f, 0.88f);
        const ImVec4 border_soft = ImVec4(0.09f, 0.16f, 0.28f, 0.72f);

        // Base colors for a darker blue-tinted theme with lighter accents
        colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);                  // Light grey text for readability
        colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.52f, 0.58f, 1.00f);          // Subtle grey-blue for disabled text
        colors[ImGuiCol_WindowBg] = base_bg;                                          // Main background stays a little darker overall
        colors[ImGuiCol_ChildBg] = base_bg_alt;                                       // Slight contrast for child windows
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);               // Slightly raised popup background
        colors[ImGuiCol_Border] = border;                                             // Dark blue border
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);          // No border shadow
        colors[ImGuiCol_FrameBg] = elevated_bg;                                       // Slightly raised control background
        colors[ImGuiCol_FrameBgHovered] = elevated_bg_hovered;                        // Subtle hover lift
        colors[ImGuiCol_FrameBgActive] = elevated_bg_active;                          // Active control surface
        colors[ImGuiCol_TitleBg] = base_bg;                                           // Title background
        colors[ImGuiCol_TitleBgActive] = base_bg_alt;                                 // Active title background
        colors[ImGuiCol_TitleBgCollapsed] = base_bg;                                  // Collapsed title background
        colors[ImGuiCol_MenuBarBg] = base_bg_alt;                                     // Menu bar background
        colors[ImGuiCol_ScrollbarBg] = base_bg_alt;                                   // Scrollbar background
        colors[ImGuiCol_ScrollbarGrab] = elevated_bg;                                 // Scrollbar grab
        colors[ImGuiCol_ScrollbarGrabHovered] = elevated_bg_hovered;                  // Scrollbar grab hover
        colors[ImGuiCol_ScrollbarGrabActive] = elevated_bg_active;                    // Scrollbar grab active
        colors[ImGuiCol_CheckMark] = accent_hovered;                                  // Lighter blue checkmark
        colors[ImGuiCol_SliderGrab] = accent;                                         // Accent slider grab
        colors[ImGuiCol_SliderGrabActive] = accent_hovered;                           // Active slider grab
        colors[ImGuiCol_Button] = accent;                                             // Lighter blue button
        colors[ImGuiCol_ButtonHovered] = accent_hovered;                              // Button hover effect
        colors[ImGuiCol_ButtonActive] = accent_active;                                // Button active state
        colors[ImGuiCol_Header] = accent_active;                                      // Header accent
        colors[ImGuiCol_HeaderHovered] = accent;                                      // Header hover effect
        colors[ImGuiCol_HeaderActive] = accent_hovered;                               // Active header
        colors[ImGuiCol_Separator] = border;                                          // Dark blue separator
        colors[ImGuiCol_SeparatorHovered] = accent_active;                            // Separator hover effect
        colors[ImGuiCol_SeparatorActive] = accent;                                    // Active separator
        colors[ImGuiCol_ResizeGrip] = accent_active;                                  // Resize grip
        colors[ImGuiCol_ResizeGripHovered] = accent;                                  // Hover effect for resize grip
        colors[ImGuiCol_ResizeGripActive] = accent_hovered;                           // Active resize grip
        colors[ImGuiCol_Tab] = base_bg_alt;                                           // Inactive tab
        colors[ImGuiCol_TabHovered] = accent;                                         // Hover effect for tab
        colors[ImGuiCol_TabActive] = accent_active;                                   // Active tab color
        colors[ImGuiCol_TabUnfocused] = base_bg;                                      // Unfocused tab
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.31f, 0.44f, 1.00f);    // Active but unfocused tab
        colors[ImGuiCol_PlotLines] = accent;                                          // Plot lines
        colors[ImGuiCol_PlotLinesHovered] = accent_hovered;                           // Hover effect for plot lines
        colors[ImGuiCol_PlotHistogram] = accent_active;                               // Histogram color
        colors[ImGuiCol_PlotHistogramHovered] = accent_hovered;                       // Hover effect for histogram
        colors[ImGuiCol_TableHeaderBg] = elevated_bg;                                 // Table header background
        colors[ImGuiCol_TableBorderStrong] = border;                                  // Strong dark blue border for tables
        colors[ImGuiCol_TableBorderLight] = border_soft;                              // Light dark-blue border for tables
        colors[ImGuiCol_TableRowBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);            // Table row background
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);         // Alternate row background
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.37f, 0.61f, 0.92f, 0.30f);        // Selected text background
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.68f, 0.97f, 0.90f);        // Drag and drop target
        colors[ImGuiCol_NavHighlight] = accent_hovered;                               // Navigation highlight
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);     // Dim background for windowing
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.06f, 0.10f, 0.45f);      // Dim background for modal windows

        // Style adjustments
        style->WindowPadding = ImVec2(8.00f, 8.00f);
        style->FramePadding = ImVec2(5.00f, 2.00f);
        style->CellPadding = ImVec2(6.00f, 6.00f);
        style->ItemSpacing = ImVec2(6.00f, 6.00f);
        style->ItemInnerSpacing = ImVec2(6.00f, 6.00f);
        style->TouchExtraPadding = ImVec2(0.00f, 0.00f);
        style->IndentSpacing = 25;
        style->ScrollbarSize = 11;
        style->GrabMinSize = 10;
        style->WindowBorderSize = 1;
        style->ChildBorderSize = 1;
        style->PopupBorderSize = 1;
        style->FrameBorderSize = 1;
        style->TabBorderSize = 1;
        style->WindowRounding = 7;
        style->ChildRounding = 4;
        style->FrameRounding = 3;
        style->PopupRounding = 4;
        style->ScrollbarRounding = 9;
        style->GrabRounding = 3;
        style->LogSliderDeadzone = 4;
        style->TabRounding = 4;
    }
}
