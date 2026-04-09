//
// Created by Marco Stulic on 4/8/26.
//

#pragma once
#include <unordered_map>
#include <pybind11/embed.h>

namespace aims {
    class PythonEventRegistrar {
    public:
        static void register_listener(const std::string& event_name, const pybind11::function& listener);

        static std::vector<pybind11::function> get_listeners(const std::string& event_name);
        static const std::unordered_multimap<std::string, pybind11::function>& get_all_listeners();
        static void run_scripts();
    protected:
        static std::recursive_mutex mutex;
        static std::unordered_multimap<std::string, pybind11::function> event_listeners;
    };
} // aims