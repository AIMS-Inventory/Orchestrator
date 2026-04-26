#include "FaceRecognitionDebug.hpp"
#include <imgui.h>
#include "FacialRecognition.hpp"

void draw_face_debug_ui() {
    auto people = FacialRecognition::get_people_on_screen();
    ImGui::Text("Faces detected: %zu", people.size());
    for (const auto& p : people) {
        ImGui::Text("Person: %s (Conf: %.2f)", p.name.c_str(), p.confidence);
    }
}
