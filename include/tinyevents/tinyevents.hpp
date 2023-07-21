#pragma once

#include <functional>
#include <list>
#include <map>
#include <typeindex>
#include <utility>

namespace tinyevents
{
    class ListenerHandle {
    public:
        explicit ListenerHandle(std::uint64_t id) : id(id) {}

        [[nodiscard]] std::uint64_t value() const { return id; }

        friend constexpr bool operator== (const ListenerHandle& lhs, const ListenerHandle& rhs) {
            return lhs.id == rhs.id;
        }
        friend constexpr bool operator!= (const ListenerHandle& lhs, const ListenerHandle& rhs) {
            return lhs.id != rhs.id;
        }
        friend constexpr bool operator< (const ListenerHandle& lhs, const ListenerHandle& rhs) {
            return lhs.id < rhs.id;
        }

    private:
        std::uint64_t id;
    };

    class Dispatcher {
        using Listeners = std::map<ListenerHandle, std::function<void(const void *)>>;
    public:
        Dispatcher() = default;
        Dispatcher(Dispatcher &&) noexcept = default;
        Dispatcher(const Dispatcher &) = delete;
        Dispatcher &operator=(Dispatcher &&) noexcept = default;

        template<typename T>
        ListenerHandle listen(const std::function<void(const T &)> &listener) {
            auto& listeners = listenersByType[std::type_index(typeid(T))];
            const auto listenerHandle = ListenerHandle{nextListenerId++};

            listeners[listenerHandle] = [listener](const auto &msg) {
                const T *concreteMessage = static_cast<const T *>(msg);
                listener(*concreteMessage);
            };
            return listenerHandle;
        }

        template<typename T>
        void send(const T &msg) {
            const auto &listeners = listenersByType.find(std::type_index(typeid(T)));
            if (listeners != listenersByType.end()) {
                for (auto &[handle, listener]: listeners->second) {
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

        void remove(const ListenerHandle &handle) {
            for (auto &listeners: listenersByType) {
                listeners.second.erase(handle);
            }
        }

        [[nodiscard]] bool hasListener(const ListenerHandle& handle) const {
            return std::any_of(listenersByType.begin(), listenersByType.end(), [&handle](const auto& listeners) {
                return listeners.second.find(handle) != listeners.second.end();
            });
        }

    private:
        std::map<std::type_index, Listeners> listenersByType;
        std::list<std::function<void(Dispatcher&)>> queuedDispatches;

        std::uint64_t nextListenerId = 0;
    };
}// namespace tinyevents