//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include "../models/Code.hpp"

class CodeDetector {
public:
    static void initialize();
    static void shutdown();
    static void main();

    static void draw_debug_ui();

    static std::vector<Code> get_current_codes();

protected:
    static std::recursive_mutex mutex;
    static std::thread* processing_thread;
    static std::atomic<bool> should_run;
    static std::vector<Code> current_codes;
};
