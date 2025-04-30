#pragma once

#include <functional>
#include <queue>
#include <set>
#include <map>
#include <typeindex>
#include <utility>

namespace tinyevents
{
    class Token;

    class Dispatcher {
        using ListenerHandle = std::uint64_t;
        using Listeners = std::map<ListenerHandle, std::function<void(const void *)>>;
    public:
        Dispatcher() = default;
        Dispatcher(Dispatcher &&) noexcept = default;
        Dispatcher(const Dispatcher &) = delete;
        Dispatcher &operator=(Dispatcher &&) noexcept = default;

        template<typename T>
        std::uint64_t listen(const std::function<void(const T &)> &listener) {
            auto& listeners = listenersByType[std::type_index(typeid(T))];
            const auto listenerHandle = ListenerHandle{nextListenerId++};

            listeners[listenerHandle] = [listener](const auto &msg) {
                const T *concreteMessage = static_cast<const T *>(msg);
                listener(*concreteMessage);
            };
            return listenerHandle;
        }

        template<typename T>
        std::uint64_t listenOnce(const std::function<void(const T &)> &listener) {
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
        void dispatch(const T &msg) {
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
            queuedDispatches.push([msg](Dispatcher& dispatcher) {
                dispatcher.dispatch(msg);
            });
        }

        void process() {
            while (!queuedDispatches.empty()) {
                std::function<void(Dispatcher&)> queuedDispatch = queuedDispatches.front();
                queuedDispatches.pop();
                queuedDispatch(*this);
            }
        }

        void remove(const std::uint64_t handle) {
            if (isScheduledForRemoval(handle)) {
                return;
            }

            for (auto &listeners: listenersByType) {
                listeners.second.erase(handle);
            }
        }

        [[nodiscard]] bool hasListener(std::uint64_t handle) const {
            if (isScheduledForRemoval(handle)) {
                return false;
            }

            return std::any_of(listenersByType.begin(), listenersByType.end(), [&handle](const auto& listeners) {
                return listeners.second.find(handle) != listeners.second.end();
            });
        }

    private:
        [[nodiscard]] bool isScheduledForRemoval(const std::uint64_t handle) const {
            return listenersScheduledForRemoval.find(handle) != listenersScheduledForRemoval.end();
        }

        std::map<std::type_index, Listeners> listenersByType;
        std::queue<std::function<void(Dispatcher&)>> queuedDispatches;
        std::set<ListenerHandle> listenersScheduledForRemoval;

        std::uint64_t nextListenerId = 0;
    };

    // RAII wrapper for listener handle.
    class Token {
    public:
        Token(Dispatcher& dispatcher, const std::uint64_t handle)
            : dispatcher(dispatcher), _handle(handle), holdsResource(true) {}
        ~Token() {
            if (holdsResource) {
                dispatcher.get().remove(_handle);
            }
        }

        // Disable copy operations
        Token(const Token&) = delete;
        Token& operator=(const Token&) = delete;

        // Enable move operations
        Token(Token&& other) noexcept
            : dispatcher(other.dispatcher), _handle(other._handle), holdsResource(other.holdsResource) {
            other.holdsResource = false;
        }

        Token& operator=(Token&& other) noexcept {
            if (this != &other) {
                if (this->holdsResource) {
                    dispatcher.get().remove(_handle);
                }
                dispatcher = other.dispatcher;
                _handle = other._handle;
                holdsResource = other.holdsResource;
                other.holdsResource = false;
            }
            return *this;
        }

        [[nodiscard]] std::uint64_t handle() const {
            return _handle;
        }

        void remove() {
            dispatcher.get().remove(_handle);
            holdsResource = false;
        }

    private:
        std::reference_wrapper<Dispatcher> dispatcher;
        std::uint64_t _handle;
        bool holdsResource;
    };
}// namespace tinyevents