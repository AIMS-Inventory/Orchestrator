//
// Created by Marco Stulic on 3/18/26.
//

#pragma once
#include <functional>
#include <mutex>
#include <vector>

namespace aims {
    template <typename... Args>
    class Event {
    public:
        using Fn = std::function<void(Args...)>;
        virtual ~Event() = default;
        void subscribe(const Fn& listener) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            listeners.push_back(listener);
        }

        void trigger(Args... args) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            for (const auto& listener : listeners) {
                listener(args...);
            }
        }

    protected:
        mutable std::recursive_mutex mutex;
        std::vector<Fn> listeners;
    };
} // aims
