//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>

namespace aims {
    struct KnownBox {
        std::string name;
        int code;
    };

    class KioskNetworkServer {
    public:
        KioskNetworkServer();
        ~KioskNetworkServer();

        void start();
        void stop();

        bool get_is_running() const;
        int get_port();

    protected:
        void update_loop();
        void on_message(const std::shared_ptr<ix::ConnectionState>& connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg);

        ix::WebSocketServer server;
        std::thread update_thread;
        std::atomic<bool> is_running;
        
        std::vector<KnownBox> known_boxes;
    };
}
