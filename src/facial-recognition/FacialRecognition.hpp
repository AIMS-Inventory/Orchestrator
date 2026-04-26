//
// Created by Marco Stulic on 4/23/26.
//

#pragma once
#include <atomic>
#include <mutex>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <vector>

#include "PersonEntry.hpp"

class FacialRecognition {
public:
    static void initialize();
    static void shutdown();
    static void main();

    static std::vector<PersonEntry> get_people_on_screen();
    static std::vector<PersonEntry> get_all_known_people();

    static void draw_debug_ui();

protected:
    static std::recursive_mutex mutex;
    static std::thread* processing_thread;
    static std::atomic<bool> should_run;
    static std::vector<PersonEntry> known_people;
    static std::vector<PersonEntry> active_people;
};
