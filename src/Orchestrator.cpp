//
// Created by user on 3/14/26.
//

#include "Orchestrator.hpp"
#include "glad/gl.h"

#include <print>
#include <thread>
#include <SDL3/SDL.h>

#include "imgui_internal.h"
#include "events/PythonEventRegistrar.hpp"
#include "events/DebugEventUI.hpp"
#include "graphics/GraphicsInitialize.hpp"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include "io/CameraInput.hpp"
#include "io/FileIo.hpp"
#include "facial-recognition/FacialRecognition.hpp"
#include "code-detection/CodeDetector.hpp"

namespace aims
{
    Orchestrator* Orchestrator::instance = nullptr;
    Orchestrator::Orchestrator() : mutex(), graphics_context(nullptr), has_graphics_context(false) {
        instance = this;
    }

    Orchestrator::~Orchestrator() = default;

    void Orchestrator::init() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto graphx_result = aims_graphx::initialize();
        if (!graphx_result) {
            std::string error_message = graphx_result.error();
            std::print("Graphics initialization failed: {}\n", error_message);
        } else {
            graphics_context = std::make_shared<aims_graphx::GraphicsContext>(graphx_result.value());
            has_graphics_context = true;
        }
        aims::load_cameras();

        FacialRecognition::initialize();
        CodeDetector::initialize();

        aims::PythonEventRegistrar::run_scripts();
    }

    void Orchestrator::run() {
        std::shared_ptr<aims::View> view = get_view("default");
        std::shared_ptr<aims::CameraInput> cam_input = std::make_shared<aims::CameraInput>(view, "default");
        std::thread([cam_input, this]() {
            while (should_run) {
                add_camera_view(cam_input);
            }
        }).detach();

        while (should_run) {
            auto context = get_graphics_context();
            if (context) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                    if (event.type == SDL_EVENT_QUIT) {
                        should_run = false;
                    }
                }

                // render
                glClearColor(0.06f, 0.07f, 0.10f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL3_NewFrame();
                ImGui::NewFrame();

                render_frame();

                ImGui::Render();
                ImGui::UpdatePlatformWindows();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                SDL_GL_SwapWindow(context->window);
            }
        }
    }

    void Orchestrator::render_frame() {
        ImGuiWindowFlags root_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        root_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        root_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        bool open = true;
        ImGui::Begin("MyDockSpaceWindow", &open, root_flags);
        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_id_right;

            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, &dock_id_right, &dock_main_id);

            ImGuiID dock_id_options;
            ImGuiID dock_id_debug;
            ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.5f, &dock_id_debug, &dock_id_options);

            ImGui::DockBuilderDockWindow("Options", dock_id_options);
            ImGui::DockBuilderDockWindow("Debug", dock_id_debug);
            ImGui::DockBuilderDockWindow("Preview", dock_main_id);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        if (ImGui::Begin("Options")) {
            auto current_active_view = get_active_view();
            const std::string current_active_label = current_active_view ? current_active_view->get_id() : "Select View";

            if (ImGui::BeginCombo("View", current_active_label.c_str())) {
                auto current_views = get_views_snapshot();
                std::shared_ptr<View> selected_view = current_active_view;

                for (const auto& view : current_views) {
                    const auto view_id = view->get_id();
                    const bool is_selected = (current_active_view && view_id == current_active_view->get_id());
                    if (ImGui::Selectable(view_id.c_str(), is_selected)) {
                        selected_view = view;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                if (selected_view != current_active_view) {
                    set_active_view(selected_view);
                    current_active_view = selected_view;
                }

                ImGui::EndCombo();
            }
        }
        ImGui::End();

        if (ImGui::Begin("Preview")) {
            auto current_active_view = get_active_view();
            if (current_active_view) {
                auto texture_handle = current_active_view->get_texture();
                float aspect_ratio = current_active_view->get_aspect_ratio();
                ImVec2 available_size = ImGui::GetContentRegionAvail();
                
                // Calculate size to fit within available space while maintaining aspect ratio
                float display_width = available_size.x;
                float display_height = display_width / aspect_ratio;
                
                if (display_height > available_size.y) {
                    display_height = available_size.y;
                    display_width = display_height * aspect_ratio;
                }
                
                ImGui::Image((void*)(intptr_t)texture_handle.texture, ImVec2(display_width, display_height));
            } else {
                ImGui::Text("No active view selected.");
            }
        }
        ImGui::End();

        if (ImGui::Begin("Debug")) {
            DebugEventUI::render_ui();
            ImGui::Separator();

            FacialRecognition::draw_debug_ui();
            ImGui::Separator();
            CodeDetector::draw_debug_ui();
        }
        ImGui::End();

        ImGui::End();
    }

    std::shared_ptr<View> Orchestrator::get_view(std::string id) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto result = std::ranges::find_if(views, [&id](auto view) {
            return view->get_id() == id;
        });
        if (result == views.end()) {
            auto view = std::make_shared<View>(id);
            views.push_back(view);
            return view;
        } else {
            return *result;
        }
    }

    void Orchestrator::push_view(const std::shared_ptr<View>& view) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        views.push_back(view);
    }

    void Orchestrator::pop_view(std::string id) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        views.erase(std::ranges::remove_if(views,
                                           [&id](const std::shared_ptr<View>& view) { return view->get_id() == id; }).begin(),
                    views.end());
    }

    void Orchestrator::add_camera_view(const std::shared_ptr<CameraInput>& cam_input) {
        cv::Mat test_image;
        if (!aims::read_camera(cam_input->get_camera_id(), test_image) || test_image.empty()) {
            std::print("Error: Grabbed an empty frame from the camera.\n");
            return;
        }

        auto view = cam_input->get_view();
        view->new_from_cv(test_image);
    }

    void Orchestrator::shutdown() {
        FacialRecognition::shutdown();
        CodeDetector::shutdown();

        std::lock_guard<std::recursive_mutex> lock(mutex);
        aims_graphx::destroy(graphics_context);
    }

    std::vector<std::shared_ptr<View>> Orchestrator::get_views_snapshot() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return views;
    }

    std::shared_ptr<View> Orchestrator::get_active_view() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return active_view;
    }

    void Orchestrator::set_active_view(const std::shared_ptr<View>& view) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        active_view = view;
    }

    std::shared_ptr<aims_graphx::GraphicsContext> Orchestrator::get_graphics_context() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return has_graphics_context ? graphics_context : nullptr;
    }

    std::vector<Code> Orchestrator::get_codes() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return current_codes;
    }

    void Orchestrator::set_codes(const std::vector<Code>& new_codes) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        current_codes = new_codes;
    }

    Orchestrator* Orchestrator::get_instance() {
        return instance;
    }

    Orchestrator& orchestrator() {
        return *Orchestrator::get_instance();
    }
}
