//
// Created by Marco Stulic on 4/9/26.
//

#include "DebugEventUI.hpp"
#include "PythonEventRegistrar.hpp"
#include <imgui.h>
#include <print>
#include <set>
#include <pybind11/embed.h>

namespace aims {
    std::string DebugEventUI::selected_event;
    int DebugEventUI::selected_listener_index = -1;

    void DebugEventUI::render_ui() {
        std::set<std::string> event_names;
        {
            auto& event_listeners = PythonEventRegistrar::get_all_listeners();
            for (const auto& [event_name, _] : event_listeners) {
                event_names.insert(event_name);
            }
        }

        ImGui::Text("Registered Events: %zu", event_names.size());
        ImGui::Separator();

        // event selection
        if (ImGui::BeginCombo("Event", selected_event.empty() ? "Select Event" : selected_event.c_str())) {
            for (const auto& event_name : event_names) {
                bool is_selected = (selected_event == event_name);
                if (ImGui::Selectable(event_name.c_str(), is_selected)) {
                    selected_event = event_name;
                    selected_listener_index = -1;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        if (!selected_event.empty()) {
            auto listeners = PythonEventRegistrar::get_listeners(selected_event);
            ImGui::Text("Listeners for '%s': %zu", selected_event.c_str(), listeners.size());

            if (ImGui::BeginListBox("##listeners", ImVec2(-1.0f, 150.0f))) {
                for (size_t i = 0; i < listeners.size(); ++i) {
                    std::string label = "Listener " + std::to_string(i);
                    bool is_selected = (selected_listener_index == static_cast<int>(i));
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        selected_listener_index = static_cast<int>(i);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::Separator();
            if (ImGui::Button("Trigger Selected Listener", ImVec2(-1.0f, 0.0f))) {
                if (selected_listener_index >= 0 && selected_listener_index < static_cast<int>(listeners.size())) {
                    try {
                        listeners[selected_listener_index]();
                        ImGui::OpenPopup("Trigger Success");
                    } catch (const std::exception& e) {
                        ImGui::OpenPopup("Trigger Error");
                        std::print("Error triggering listener {} for event '{}': {}\n", selected_listener_index, selected_event, e.what());
                    }
                }
            }

            if (ImGui::Button("Trigger All Listeners", ImVec2(-1.0f, 0.0f))) {
                try {
                    for (const auto& listener : listeners) {
                        listener();
                    }
                    ImGui::OpenPopup("Trigger Success");
                } catch (const std::exception& e) {
                    ImGui::OpenPopup("Trigger Error");
                }
            }
        }

        if (ImGui::BeginPopupModal("Trigger Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Event triggered successfully!");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Trigger Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error triggering event!");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
} // aims


