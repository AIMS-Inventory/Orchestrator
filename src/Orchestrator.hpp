//
// Created by user on 3/14/26.
//

#pragma once
#include <memory>
#include <mutex>

#include "graphics/GraphicsInitialize.hpp"
#include "graphics/View.hpp"

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

        void cv_display_test();

        std::shared_ptr<View> get_view(std::string id);
        void push_view(const std::shared_ptr<View>& view);
        void pop_view(std::string id);

        static Orchestrator* get_instance();
    protected:
        mutable std::recursive_mutex mutex;

        static Orchestrator* instance;
        std::shared_ptr<aims_graphx::GraphicsContext> graphics_context;
        bool has_graphics_context = false;
        bool should_run = true;
        std::shared_ptr<View> active_view;
        std::vector<std::shared_ptr<View>> views;
    };

    Orchestrator& orchestrator();
}
