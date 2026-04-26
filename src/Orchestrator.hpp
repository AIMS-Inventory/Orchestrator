//
// Created by user on 3/14/26.
//

#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "graphics/GraphicsInitialize.hpp"
#include "graphics/View.hpp"
#include "io/CameraInput.hpp"
#include "models/Code.hpp"
#include "models/Box.hpp"
#include "models/Shelf.hpp"
#include "io/KioskNetworkServer.hpp"

namespace aims
{
    class Orchestrator {
    public:
        Orchestrator();
        ~Orchestrator();

        void init();
        void render_frame();
        void run();
        void shutdown();

        void add_camera_view(const std::shared_ptr<CameraInput>& cam_input);

        std::vector<Box> get_boxes();
        std::vector<Shelf> get_shelves();

        std::shared_ptr<View> get_view(std::string id);
        void push_view(const std::shared_ptr<View>& view);
        void pop_view(std::string id);

        std::vector<Code> get_codes();
        void set_codes(const std::vector<Code>& new_codes);

        static Orchestrator* get_instance();
    protected:
        std::vector<std::shared_ptr<View>> get_views_snapshot();
        std::shared_ptr<View> get_active_view();
        void set_active_view(const std::shared_ptr<View>& view);
        std::shared_ptr<aims_graphx::GraphicsContext> get_graphics_context();

        mutable std::recursive_mutex mutex;

        static Orchestrator* instance;
        std::shared_ptr<aims_graphx::GraphicsContext> graphics_context;
        bool has_graphics_context = false;
        std::atomic<bool> should_run = true;

        KioskNetworkServer network_server;

        std::vector<Box> boxes;
        std::vector<Shelf> shelves;

        std::shared_ptr<View> active_view;
        std::vector<std::shared_ptr<View>> views;

        std::vector<Code> current_codes;
    };

    Orchestrator& orchestrator();
}
