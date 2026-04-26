//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

namespace aims {
    class KioskNetworkServer {
    public:
        KioskNetworkServer();
        ~KioskNetworkServer();

        void start();
        void stop();

    protected:
        void update_loop();
        void on_message(std::shared_ptr<ix::ConnectionState> connectionState, const ix::WebSocketMessagePtr& msg);

        ix::WebSocketServer server;
        std::thread update_thread;
        std::atomic<bool> is_running;
    };
}
