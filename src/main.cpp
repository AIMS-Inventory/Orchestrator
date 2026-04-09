#include <print>
#include <string>
#include <pybind11/embed.h>

#include "Orchestrator.hpp"

int main() {
    pybind11::scoped_interpreter guard{};
    aims::Orchestrator orchestrator;
    orchestrator.init();
    orchestrator.run();
    orchestrator.shutdown();
    return 0;
}
