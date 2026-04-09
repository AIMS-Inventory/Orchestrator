//
// Created by Marco Stulic on 4/8/26.
//

#include "PythonEventRegistrar.hpp"

#include <iostream>
#include <pybind11/embed.h>
#include <__filesystem/filesystem_error.h>

#include "../io/FileIo.hpp"

namespace aims {
    std::recursive_mutex PythonEventRegistrar::mutex;
    std::unordered_multimap<std::string, pybind11::function> PythonEventRegistrar::event_listeners;

    void PythonEventRegistrar::register_listener(const std::string &event_name, const pybind11::function &listener) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            event_listeners.emplace(event_name, listener);
    }

    std::vector<pybind11::function> PythonEventRegistrar::get_listeners(const std::string &event_name) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        std::vector<pybind11::function> listeners;
        auto range = event_listeners.equal_range(event_name);
        for (auto it = range.first; it != range.second; ++it) {
            listeners.push_back(it->second);
        }
        return listeners;
    }

    const std::unordered_multimap<std::string, pybind11::function>& PythonEventRegistrar::get_all_listeners() {
        return event_listeners;
    }

    void PythonEventRegistrar::run_scripts() {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        std::vector<std::filesystem::path> script_paths = aims::enumerate_scripts();
        for (const auto& script_path : script_paths) {
            try {
                std::string script_str = script_path;
                pybind11::eval_file(script_str);
                std::print("Successfully ran script '{}'\n", script_path.string());
            } catch (const pybind11::error_already_set& e) {
                std::print("Error running script '{}': {}\n", script_path.string(), e.what());
            } catch (const std::filesystem::filesystem_error& e) {
                std::print("Filesystem error while accessing script '{}': {}\n", script_path.string(), e.what());
            } catch (const std::exception& e) {
                std::print("Unexpected error while running script '{}': {}\n", script_path.string(), e.what());
            }
        }
        std::print("Ran all scripts!\n");
    }


} // aims

PYBIND11_EMBEDDED_MODULE(aims_py, m) {
    m.def("register_listener", &aims::PythonEventRegistrar::register_listener, "Register a Python listener for an event");
}