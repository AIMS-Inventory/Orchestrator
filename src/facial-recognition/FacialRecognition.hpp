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
protected:
    static std::recursive_mutex mutex;
    static std::thread* processing_thread;
    static std::atomic<bool> should_run;
    static std::vector<PersonEntry> known_people;
};

