//
// Created by Marco Stulic on 4/26/26.
//

#include "KioskNetworkServer.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <unordered_set>
#include "../events/PythonEventRegistrar.hpp"
#include "../Orchestrator.hpp"
#include "../facial-recognition/FacialRecognition.hpp"
#include "../pills/PillRecognition.hpp"
#include "FileIo.hpp"
#include <ryml.hpp>
#include <ryml_std.hpp>

namespace aims {

    KioskNetworkServer::KioskNetworkServer() : server(8001, "0.0.0.0"), is_running(false) {
        server.setOnClientMessageCallback([this](const std::shared_ptr<ix::ConnectionState>& connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
            this->on_message(connectionState, webSocket, msg);
        });

        try {
            auto tree = aims::parse_config("known_box_codes.yml");
            auto root = tree.rootref();
            if (!root.invalid() && root.has_child("boxes") && root["boxes"].is_seq()) {
                for (auto node : root["boxes"]) {
                    KnownBox kb;
                    if (node.has_child("name")) {
                        node["name"] >> kb.name;
                    }
                    if (node.has_child("code")) {
                        std::string code_str;
                        node["code"] >> code_str;
                        kb.code = std::stoi(code_str);
                    }
                    known_boxes.push_back(kb);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to load known_box_codes.yml: " << e.what() << std::endl;
        }
    }

    KioskNetworkServer::~KioskNetworkServer() {
        stop();
    }

    void KioskNetworkServer::start() {
        if (is_running) {
            std::cout << "[DEBUG] KioskNetworkServer is already running." << std::endl;
            return;
        }
        
        std::cout << "[DEBUG] Attempting to listen on port 8001..." << std::endl;
        auto res = server.listen();
        if (!res.first) {
            std::cerr << "[ERROR] KioskNetworkServer failed to listen: " << res.second << std::endl;
            return;
        }

        is_running = true;
        server.start();
        update_thread = std::thread(&KioskNetworkServer::update_loop, this);
        std::cout << "[DEBUG] KioskNetworkServer started successfully." << std::endl;
    }

    void KioskNetworkServer::stop() {
        if (!is_running) {
            std::cout << "[DEBUG] KioskNetworkServer is already stopped." << std::endl;
            return;
        }
        std::cout << "[DEBUG] Stopping KioskNetworkServer..." << std::endl;
        is_running = false;
        server.stop();
        if (update_thread.joinable()) {
            update_thread.join();
        }
        std::cout << "[DEBUG] KioskNetworkServer stopped." << std::endl;
    }

    bool KioskNetworkServer::get_is_running() const {
        return is_running;
    }

    int KioskNetworkServer::get_port() {
        return server.getPort();
    }

    void KioskNetworkServer::update_loop() {
        while (is_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            nlohmann::json packet;
            // unknown_box_ids
            packet["unknown_box_ids"] = nlohmann::json::array();
            auto codes = aims::orchestrator().get_codes();
            auto shelves = aims::orchestrator().get_shelves();
            auto boxes = aims::orchestrator().get_boxes();

            std::unordered_set<std::string> known_box_ids;
            for (const auto& box : boxes) {
                known_box_ids.insert(box.id);
            }
            for (const auto& shelf : shelves) {
                for (const auto& [_, box] : shelf.boxes) {
                    known_box_ids.insert(box.id);
                }
            }

            for (const auto& code : codes) {
                bool is_shelf = false;
                for (const auto& shelf : shelves) {
                    if (shelf.code == code.id) {
                        is_shelf = true;
                        break;
                    }
                }
                
                const bool is_box = known_box_ids.contains(std::to_string(code.id));

                if (!is_shelf && !is_box) {
                    packet["unknown_box_ids"].push_back(code.id);
                }
            }

            // recognized_faces
            packet["recognized_faces"] = nlohmann::json::object();
            for (const auto& person : FacialRecognition::get_people_on_screen()) {
                nlohmann::json person_json;
                person_json["name"] = person.name;
                person_json["id"] = person.id;
                person_json["last_seen"] = person.last_seen;
                person_json["confidence"] = person.confidence;
                person_json["extra_info"] = person.extra_info;
                packet["recognized_faces"][std::to_string(person.id)] = person_json;
            }

            // crew_info
            packet["crew_info"] = nlohmann::json::object();
            for (const auto& person : FacialRecognition::get_all_known_people()) {
                nlohmann::json person_json;
                person_json["name"] = person.name;
                person_json["id"] = person.id;
                person_json["last_seen"] = person.last_seen;
                person_json["confidence"] = person.confidence;
                person_json["extra_info"] = person.extra_info;
                packet["crew_info"][std::to_string(person.id)] = person_json;
            }

            // shelf_info
            packet["shelf_info"] = nlohmann::json::object();
            for (const auto& shelf : shelves) {
                nlohmann::json shelf_json;
                shelf_json["name"] = shelf.name;
                shelf_json["code"] = shelf.code;
                
                nlohmann::json boxes_json = nlohmann::json::object();
                for (const auto& [pos, box] : shelf.boxes) {
                    boxes_json[pos] = box.id;
                }
                shelf_json["boxes"] = boxes_json;
                
                packet["shelf_info"][std::to_string(shelf.code)] = shelf_json;
            }

            // box_info
            packet["box_info"] = nlohmann::json::object();
            for (const auto& box : boxes) {
                nlohmann::json box_json;
                box_json["id"] = box.id;
                nlohmann::json contents;
                contents["placed_by"] = box.contents.placed_by;
                contents["pills"] = box.contents.pills;
                box_json["contents"] = contents;
                
                packet["box_info"][box.id] = box_json;
            }

            // known_boxes
            packet["known_boxes"] = nlohmann::json::array();
            for (const auto& kb : known_boxes) {
                nlohmann::json kb_json;
                kb_json["name"] = kb.name;
                kb_json["code"] = kb.code;
                packet["known_boxes"].push_back(kb_json);
            }

            std::string payload = packet.dump();

            for (const auto& client : server.getClients()) {
                client->sendText(payload);
            }
        }
    }

    void KioskNetworkServer::on_message(const std::shared_ptr<ix::ConnectionState>& connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
        (void)connectionState;
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::cout << "[DEBUG] Received message from client: " << msg->str << std::endl;
            try {
                nlohmann::json payload = nlohmann::json::parse(msg->str);
                std::string event_type = payload.value("event", "");
                std::cout << "got event type: " << event_type << std::endl;
                if (!event_type.empty()) {
                    if (event_type == "get_on_screen_pills") {
                        nlohmann::json response;
                        response["event"] = "on_screen_pills";

                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }

                        nlohmann::json pills_json = nlohmann::json::array();
                        for (const auto& pill : PillRecognition::recognize_pills()) {
                            nlohmann::json item;
                            item["name"] = pill.name;
                            item["quantity"] = pill.quantity;
                            item["confidence"] = pill.confidence;
                            item["bounds"] = {
                                {"x", pill.bounds.x},
                                {"y", pill.bounds.y},
                                {"width", pill.bounds.width},
                                {"height", pill.bounds.height}
                            };
                            pills_json.push_back(item);
                        }

                        response["pills"] = pills_json;
                        webSocket.sendText(response.dump());
                        return;
                    }

                    if (event_type == "clear_shelf") {
                        nlohmann::json response;
                        response["event"] = "clear_shelf_result";
                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }
                        int shelf_code = payload.value("shelf_code", -1);
                        bool success = aims::orchestrator().clear_shelf(shelf_code);
                        response["success"] = success;
                        response["shelf_code"] = shelf_code;
                        webSocket.sendText(response.dump());
                        return;
                    }

                    if (event_type == "get_audit_logs") {
                        nlohmann::json response;
                        response["event"] = "audit_logs";
                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }
                        nlohmann::json logs = nlohmann::json::array();

                        std::string audit_path = "aimsai/aimsys/system_audit_alpha.csv";
                        std::ifstream file(audit_path);
                        if (file.is_open()) {
                            std::string line;
                            if (std::getline(file, line)) { // skip header
                                while (std::getline(file, line)) {
                                    if (line.empty()) continue;
                                    std::stringstream ss(line);
                                    std::string item;
                                    nlohmann::json log_entry;

                                    if (std::getline(ss, item, ',')) log_entry["timestamp"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["crew_member_name"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["box_id"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["shelf_id"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["pill_type"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["pill_quantity_before"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["pill_quantity_after"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["registered_at"] = item;
                                    if (std::getline(ss, item, ',')) log_entry["error_detail"] = item;

                                    logs.push_back(log_entry);
                                }
                            }
                        } else {
                            std::cerr << "Failed to open audit file: " << audit_path << std::endl;
                        }
                        response["logs"] = logs;
                        webSocket.sendText(response.dump());
                        return;
                    }

                    if (event_type == "generate_aimsai_report") {
                        nlohmann::json response;
                        response["event"] = "generate_aimsai_report_result";
                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }

                        std::string filename = payload.value("filename", "report.csv");
                        std::string content  = payload.value("content", "");
                        std::string report_path = "aimsai/aimsys/" + filename;

                        std::cout << "[AIMSAI] ── Report Generation Started ──────────────────" << std::endl;
                        std::cout << "[AIMSAI] Filename  : " << filename << std::endl;
                        std::cout << "[AIMSAI] Full path : " << report_path << std::endl;
                        std::cout << "[AIMSAI] CSV size  : " << content.size() << " bytes" << std::endl;

                        int row_count = 0;
                        for (char c : content) { if (c == '\n') ++row_count; }
                        if (row_count > 0) --row_count;
                        std::cout << "[AIMSAI] CSV rows  : " << row_count << " (excl. header)" << std::endl;

                        std::ofstream out(report_path);
                        bool success = false;
                        if (out.is_open()) {
                            out << content;
                            out.close();
                            std::cout << "[AIMSAI] CSV written successfully." << std::endl;

                            std::string exec_cmd = "cd aimsai && ./aims-ai";
                            std::cout << "[AIMSAI] Launching : " << exec_cmd << std::endl;

                            auto exec_start = std::chrono::steady_clock::now();
                            int ret = std::system(exec_cmd.c_str());
                            auto exec_end = std::chrono::steady_clock::now();
                            long long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_end - exec_start).count();

                            if (ret == 0) {
                                std::cout << "[AIMSAI] aims-ai exited OK (code 0) in " << elapsed_ms << " ms." << std::endl;
                                success = true;
                            } else {
                                std::cerr << "[AIMSAI] aims-ai exited with code " << ret << " after " << elapsed_ms << " ms." << std::endl;
                            }
                        } else {
                            std::cerr << "[AIMSAI] Failed to open file for writing: " << report_path << std::endl;
                        }

                        std::cout << "[AIMSAI] Result    : " << (success ? "SUCCESS" : "FAILURE") << std::endl;
                        std::cout << "[AIMSAI] ── Report Generation Complete ─────────────────" << std::endl;

                        response["success"] = success;
                        response["path"] = report_path;
                        webSocket.sendText(response.dump());
                        return;
                    }

                    if (event_type == "download_report") {
                        nlohmann::json response;
                        response["event"] = "report_data";
                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }

                        std::string forecast_path = "aimsai/forecasting/output.yaml";
                        std::ifstream file(forecast_path);
                        std::string content;
                        bool success = false;

                        if (file.is_open()) {
                            std::ostringstream buffer;
                            buffer << file.rdbuf();
                            content = buffer.str();
                            success = true;
                        } else {
                            std::cerr << "[ERROR] Failed to open forecast file: " << forecast_path << std::endl;
                        }

                        response["success"] = success;
                        response["content"] = content;
                        webSocket.sendText(response.dump());
                        return;
                    }

                    pybind11::gil_scoped_acquire acquire;
                    std::cout << "[DEBUG] Dispatching event '" << event_type << "' to Python listeners." << std::endl;
                    auto listeners = PythonEventRegistrar::get_listeners(event_type);
                    std::cout << "found " << listeners.size() << " listeners." << std::endl;
                    for (auto& listener : listeners) {
                        try {
                            listener(msg->str);
                        } catch (const std::exception& e) {
                            std::cerr << "Python event error: " << e.what() << std::endl;
                        }
                    }

                    if (event_type == "register_box") {
                        nlohmann::json response;
                        response["event"] = "register_box_result";
                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }

                        std::string requested_box_id;
                        if (payload.contains("box_id")) {
                            if (payload["box_id"].is_string()) {
                                requested_box_id = payload["box_id"].get<std::string>();
                            } else if (payload["box_id"].is_number_integer()) {
                                requested_box_id = std::to_string(payload["box_id"].get<long long>());
                            } else if (payload["box_id"].is_number_unsigned()) {
                                requested_box_id = std::to_string(payload["box_id"].get<unsigned long long>());
                            } else {
                                requested_box_id = payload["box_id"].dump();
                            }
                        }

                        std::string requested_position;
                        if (payload.contains("position")) {
                            if (payload["position"].is_string()) {
                                requested_position = payload["position"].get<std::string>();
                            } else if (payload["position"].is_array() && payload["position"].size() >= 2) {
                                requested_position = std::to_string(payload["position"][0].get<int>()) + "," + std::to_string(payload["position"][1].get<int>());
                            } else {
                                requested_position = payload["position"].dump();
                            }
                        }

                        int requested_shelf_code = payload.value("shelf_code", -1);
                        bool found = false;
                        nlohmann::json registered_box_json;

                        auto current_shelves = aims::orchestrator().get_shelves();
                        for (const auto& shelf : current_shelves) {
                            if (requested_shelf_code >= 0 && shelf.code != requested_shelf_code) {
                                continue;
                            }

                            if (!requested_position.empty()) {
                                auto it = shelf.boxes.find(requested_position);
                                if (it != shelf.boxes.end() && (requested_box_id.empty() || it->second.id == requested_box_id)) {
                                    registered_box_json["id"] = it->second.id;
                                    registered_box_json["shelf_code"] = shelf.code;
                                    registered_box_json["shelf_name"] = shelf.name;
                                    registered_box_json["position"] = requested_position;
                                    registered_box_json["contents"] = {
                                        {"placed_by", it->second.contents.placed_by},
                                        {"pills", it->second.contents.pills}
                                    };
                                    found = true;
                                    break;
                                }
                            }

                            for (const auto& [pos, box] : shelf.boxes) {
                                if (!requested_box_id.empty() && box.id != requested_box_id) {
                                    continue;
                                }

                                registered_box_json["id"] = box.id;
                                registered_box_json["shelf_code"] = shelf.code;
                                registered_box_json["shelf_name"] = shelf.name;
                                registered_box_json["position"] = pos;
                                registered_box_json["contents"] = {
                                    {"placed_by", box.contents.placed_by},
                                    {"pills", box.contents.pills}
                                };
                                found = true;
                                break;
                            }

                            if (found) {
                                break;
                            }
                        }

                        response["success"] = found;
                        response["registered_box"] = found ? registered_box_json : nlohmann::json(nullptr);
                        if (!found) {
                            response["error"] = "Box was not found on the requested shelf after registration.";
                        }

                        webSocket.sendText(response.dump());
                    }

                    if (event_type == "remove_box") {
                        nlohmann::json response;
                        response["event"] = "remove_box_result";
                        if (payload.contains("request_id")) {
                            response["request_id"] = payload["request_id"];
                        }

                        std::string box_id;
                        if (payload.contains("box_id")) {
                            box_id = payload["box_id"].is_string()
                                ? payload["box_id"].get<std::string>()
                                : payload["box_id"].dump();
                        }

                        bool success = false;
                        if (!box_id.empty()) {
                            int found_shelf_code = -1;
                            std::string found_position;
                            auto current_shelves = aims::orchestrator().get_shelves();
                            for (const auto& shelf : current_shelves) {
                                for (const auto& [pos, box] : shelf.boxes) {
                                    if (box.id == box_id) {
                                        found_shelf_code = shelf.code;
                                        found_position = pos;
                                        break;
                                    }
                                }
                                if (found_shelf_code >= 0) break;
                            }
                            if (found_shelf_code >= 0) {
                                success = aims::orchestrator().unregister_box_from_shelf(found_shelf_code, found_position);
                            }
                        }

                        response["success"] = success;
                        response["box_id"] = box_id;
                        if (!success) {
                            response["error"] = "Box could not be removed or was not found.";
                        }

                        webSocket.sendText(response.dump());
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "KioskNetworkServer Message Parsing Error: " << e.what() << std::endl;
            }
        }
    }

}
