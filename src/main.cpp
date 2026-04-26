#include <print>
#include <string>
#include <pybind11/embed.h>

#include "Orchestrator.hpp"

int main() {
    pybind11::scoped_interpreter guard{};
    aims::Orchestrator orchestrator;
    orchestrator.init();

    // Release the GIL so background threads can acquire it
    pybind11::gil_scoped_release release;

    orchestrator.run();
    orchestrator.shutdown();
    return 0;
}
