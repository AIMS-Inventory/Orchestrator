//
// Created by user on 3/14/26.
//

#include "Orchestrator.hpp"
#include "glad/gl.h"

#include <print>
#include <SDL3/SDL.h>

#include "imgui_internal.h"
#include "graphics/GraphicsInitialize.hpp"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_sdl3.h"

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

        cv_display_test();
    }

    void Orchestrator::run() {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        while (should_run) {
            if (has_graphics_context) {
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
                SDL_GL_SwapWindow(graphics_context->window);
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

            ImGui::DockBuilderDockWindow("Options", dock_id_right);
            ImGui::DockBuilderDockWindow("Preview", dock_main_id);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        if (ImGui::Begin("Options")) {
            if (ImGui::BeginCombo("View", active_view ? active_view->get_id().c_str() : "Select View")) {
                for (const auto& view : views) {
                    bool is_selected = (active_view && view->get_id() == active_view->get_id());
                    if (ImGui::Selectable(view->get_id().c_str(), is_selected)) {
                        active_view = view;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::End();

        if (ImGui::Begin("Preview")) {
            if (active_view) {
                auto texture_handle = active_view->get_texture();
                ImGui::Image((void*)(intptr_t)texture_handle.texture, ImVec2(512, 512));
            } else {
                ImGui::Text("No active view selected.");
            }
        }
        ImGui::End();

        ImGui::End();
    }

    std::shared_ptr<View> Orchestrator::get_view(std::string id) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        for (const auto& view : views) {
            if (view->get_id() == id) {
                return view;
            }
        }
        return nullptr;
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

    void Orchestrator::cv_display_test() {
        cv::Mat test_image(512, 512, CV_8UC3, cv::Scalar(255, 0, 0));
        auto view = std::make_shared<View>("test_view");
        view->new_from_cv(test_image);
        push_view(view);
    }

    void Orchestrator::shutdown() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        aims_graphx::destroy(graphics_context);
    }

    Orchestrator* Orchestrator::get_instance() {
        return instance;
    }

    Orchestrator& orchestrator() {
        return *Orchestrator::get_instance();
    }
}
