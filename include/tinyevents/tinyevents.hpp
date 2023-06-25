#pragma once

#include <functional>
#include <list>
#include <map>
#include <typeindex>

namespace tinyevents
{
    class Dispatcher {
    public:
        Dispatcher() = default;
        Dispatcher(Dispatcher &&) noexcept = default;
        Dispatcher(const Dispatcher &) = delete;
        Dispatcher &operator=(Dispatcher &&) noexcept = default;

        template<typename T>
        void listen(const std::function<void(const T &)> &listener) {
            listenersByType[std::type_index(typeid(T))].push_back([listener](const auto &msg) {
                const T *concreteMessage = static_cast<const T *>(msg);
                listener(*concreteMessage);
            });
        }

        template<typename T>
        void send(const T &msg) {
            const auto &listeners = listenersByType.find(std::type_index(typeid(T)));
            if (listeners != listenersByType.end()) {
                for (auto &listener: listeners->second) {
                    listener(&msg);
                }
            }
        }

        template<typename T>
        void queue(const T &msg) {
            queuedDispatches.push_back([msg](Dispatcher& dispatcher) {
                dispatcher.send(msg);
            });
        }

        void process() {
            for (auto &queuedDispatch: queuedDispatches) {
                queuedDispatch(*this);
            }
            queuedDispatches.clear();
        }

    private:
        using Listeners = std::list<std::function<void(const void *)>>;
        std::map<std::type_index, Listeners> listenersByType;
        std::list<std::function<void(Dispatcher&)>> queuedDispatches;
    };
}// namespace tinyevents