//
// Created by Marco Stulic on 4/9/26.
//

#pragma once

#include <string>
#include <vector>

namespace aims {
    class DebugEventUI {
    public:
        static void render_ui();

    private:
        static std::string selected_event;
        static int selected_listener_index;
    };
} // aims

