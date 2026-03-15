#include <print>
#include <string>

#include "Orchestrator.hpp"

int main() {
    aims::Orchestrator orchestrator;
    orchestrator.init();
    orchestrator.run();
    orchestrator.shutdown();
    return 0;
}
