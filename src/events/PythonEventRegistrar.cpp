//
// Created by Marco Stulic on 4/8/26.
//

#include "PythonEventRegistrar.hpp"

#include <iostream>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <__filesystem/filesystem_error.h>

#include "../Orchestrator.hpp"
#include "../io/FileIo.hpp"

namespace aims {
    std::recursive_mutex PythonEventRegistrar::mutex;
    std::unordered_multimap<std::string, pybind11::function> PythonEventRegistrar::event_listeners;

    void PythonEventRegistrar::register_listener(const std::string &event_name, const pybind11::function &listener) {
            pybind11::gil_scoped_acquire acquire;
            std::lock_guard<std::recursive_mutex> lock(mutex);
            event_listeners.emplace(event_name, listener);
    }

    std::vector<pybind11::function> PythonEventRegistrar::get_listeners(const std::string &event_name) {
        pybind11::gil_scoped_acquire acquire;
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
        pybind11::gil_scoped_acquire acquire;
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
    namespace py = pybind11;

    py::class_<aims::BoxContents>(m, "BoxContents")
        .def(py::init<>())
        .def_readwrite("placed_by", &aims::BoxContents::placed_by)
        .def_readwrite("pills", &aims::BoxContents::pills);

    py::class_<aims::Box>(m, "Box")
        .def(py::init<>())
        .def_readwrite("id", &aims::Box::id)
        .def_readwrite("contents", &aims::Box::contents);

    py::class_<aims::Shelf>(m, "Shelf")
        .def(py::init<>())
        .def_readwrite("name", &aims::Shelf::name)
        .def_readwrite("code", &aims::Shelf::code)
        .def_readwrite("boxes", &aims::Shelf::boxes);

    m.def("register_listener", &aims::PythonEventRegistrar::register_listener, "Register a Python listener for an event");
    m.def("register_box", [](int shelf_code, const std::string& position, const aims::Box& box) {
        py::gil_scoped_acquire acquire;
        return aims::orchestrator().register_box_to_shelf(shelf_code, position, box);
    }, "Register a box at a shelf position");
    m.def("unregister_box", [](int shelf_code, const std::string& position) {
        py::gil_scoped_acquire acquire;
        return aims::orchestrator().unregister_box_from_shelf(shelf_code, position);
    }, "Remove a box from a shelf position");
    m.def("clear_shelf", [](int shelf_code) {
        py::gil_scoped_acquire acquire;
        return aims::orchestrator().clear_shelf(shelf_code);
    }, "Clear all boxes from a shelf");
    m.def("get_shelves", []() {
        py::gil_scoped_acquire acquire;
        return aims::orchestrator().get_shelves();
    }, "Get all shelves with their current boxes");
    m.def("get_boxes", []() {
        py::gil_scoped_acquire acquire;
        return aims::orchestrator().get_boxes();
    }, "Get all boxes");
}