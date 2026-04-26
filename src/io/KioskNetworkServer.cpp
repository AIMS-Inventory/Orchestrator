//
// Created by Marco Stulic on 4/26/26.
//

#include "KioskNetworkServer.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include "../events/PythonEventRegistrar.hpp"

namespace aims {

    KioskNetworkServer::KioskNetworkServer() : server(8001, "0.0.0.0"), is_running(false) {
        server.setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg) {
            this->on_message(connectionState, msg);
        });
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
            packet["unknown_box_ids"] = nlohmann::json::array();
            packet["recognized_faces"] = nlohmann::json::object();
            packet["crew_info"] = nlohmann::json::object();
            packet["shelf_info"] = nlohmann::json::object();
            packet["box_info"] = nlohmann::json::object();

            std::string payload = packet.dump();

            for (auto client : server.getClients()) {
                client->sendText(payload);
            }
        }
    }

    void KioskNetworkServer::on_message(std::shared_ptr<ix::ConnectionState> connectionState, const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            try {
                nlohmann::json payload = nlohmann::json::parse(msg->str);
                std::string event_type = payload.value("event", "");
                if (!event_type.empty()) {
                    auto& all_listeners = PythonEventRegistrar::get_all_listeners();
                    auto range = all_listeners.equal_range(event_type);
                    for (auto it = range.first; it != range.second; ++it) {
                        try {
                            it->second(msg->str);
                        } catch (const std::exception& e) {
                            std::cerr << "Python event error: " << e.what() << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "KioskNetworkServer Message Parsing Error: " << e.what() << std::endl;
            }
        }
    }

}
