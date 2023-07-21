#pragma once

#include <functional>
#include <list>
#include <set>
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
        ListenerHandle listenOnce(const std::function<void(const T &)> &listener) {
            const auto listenerId = nextListenerId;
            return listen<T>([this, listenerId, listener](const T &msg) {
                ListenerHandle handle{listenerId};
                listenersScheduledForRemoval.emplace(handle); // Fix for nested listenOnce
                listener(msg);
                listenersScheduledForRemoval.erase(handle);
                this->remove(handle);
            });
        }

        template<typename T>
        void send(const T &msg) {
            const auto &listenersIter = listenersByType.find(std::type_index(typeid(T)));
            if (listenersIter == listenersByType.end()) {
                return; // No listeners for this type of message
            }

            const auto& [msgType, listeners] = *listenersIter;

            // Cache handles to avoid iterator invalidation. This way listeners can safely remove themselves.
            std::vector<ListenerHandle> handles;
            handles.reserve(listeners.size());
            for (auto &[handle, listener]: listeners) {
                handles.push_back(handle);
            }

            for(auto& handle: handles) {
                const auto& handleAndListener = listeners.find(handle);
                const bool isListenerPresent = handleAndListener != listeners.end();
                if (isListenerPresent && !isScheduledForRemoval(handle)) {
                    const auto& listener = handleAndListener->second;
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
            if (isScheduledForRemoval(handle)) {
                return;
            }

            for (auto &listeners: listenersByType) {
                listeners.second.erase(handle);
            }
        }

        [[nodiscard]] bool hasListener(const ListenerHandle& handle) const {
            if (isScheduledForRemoval(handle)) {
                return false;
            }

            return std::any_of(listenersByType.begin(), listenersByType.end(), [&handle](const auto& listeners) {
                return listeners.second.find(handle) != listeners.second.end();
            });
        }

    private:
        bool isScheduledForRemoval(const ListenerHandle& handle) const {
            return listenersScheduledForRemoval.find(handle) != listenersScheduledForRemoval.end();
        }

        std::map<std::type_index, Listeners> listenersByType;
        std::list<std::function<void(Dispatcher&)>> queuedDispatches;
        std::set<ListenerHandle> listenersScheduledForRemoval;

        std::uint64_t nextListenerId = 0;
    };
}// namespace tinyevents