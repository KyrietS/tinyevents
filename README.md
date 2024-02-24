# TinyEvents - A Simple Event-Dispatcher System for C++

<!--[![release](https://img.shields.io/github/v/release/KyrietS/tinyevents?include_prereleases&sort=semver)](https://github.com/KyrietS/tinyevents/releases)-->
[![Tests](https://github.com/KyrietS/tinyevents/actions/workflows/tests.yml/badge.svg)](https://github.com/KyrietS/tinyevents/actions/workflows/tests.yml)
[![Lincense](https://img.shields.io/github/license/KyrietS/tinyevents)](LICENSE)

*TinyEvents* is a simple header-only library for C++ that provides a basic, yet powerfull, event-dispatcher system. It is designed to be easy to use and to have minimal dependencies. It is written in C++17 and has no dependencies other than the standard library.

In *TinyEvents* any type can be used as an event. The events are dispatched to listeners that are registered for a specific event type. Asynchronous (deferred) dispatching using a queue is also supported. With the `tinyevents::Token` helper class you can get RAII-style automatic listener removal.

## Basic Usage

```cpp
#include <tinyevents/tinyevents.hpp>
#include <iostream>

struct MyEvent {
    int value;
};

int main() {
    tinyevents::Dispatcher dispatcher;

    // Register a listener for MyEvent
    auto handle = dispatcher.listen<MyEvent>([](const auto& event) {
        std::cout << "Received MyEvent: " << event.value << std::endl;
    });

    // Dispatch an event
    dispatcher.dispatch(MyEvent{11});  // Prints "Received MyEvent: 11"

    // Queue events
    dispatcher.queue(MyEvent{22});
    dispatcher.queue(MyEvent{33});
    dispatcher.process();              // Prints "Received MyEvent: 22"
                                       //        "Received MyEvent: 33"

    dispatcher.remove(handle);         // Remove the listener
    dispatcher.dispatch(MyEvent{44});  // No listener, so nothing happens

    return 0;
}
```

## Getting Started

### Manual Installation
Just copy the `include/tinyevents` folder to your project and add the `include` folder to your include path. The library is header-only, so no compilation is needed.

### CMake
If you use CMake, you can use the `FetchContent` module to download the library and add it to your project. For example:

```cmake
include(FetchContent)
FetchContent_Declare(
  tinyevents
  GIT_REPOSITORY https://github.com/KyrietS/tinyevents.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(tinyevents)
target_link_libraries(${TARGET} PRIVATE tinyevents)
```

Or you can just clone this repository manually and add it as a subdirectory to your CMake project.
```cmake
add_subdirectory(tinyevents)
target_link_libraries(${TARGET} PRIVATE tinyevents)
```

Don't forget to change the `${TARGET}` to the name of your target.

## Documentation

### Listening to Events

Register a listener for a specific event type. The listener will be called when an event of the specified type is dispatched.

```cpp
template<typename Event>
std::uint64_t listen(const std::function<void(const Event&)>& listener)
```
* listener - A callable object that will be called when an event of type `Event` is dispatched. The object must be copyable.

Returns a handle that can be used to remove the listener.

### Listening to Events Once

Same as `listen()`, but the listener will be removed after it is called once.

```cpp
template<typename Event>
std::uint64_t listenOnce(const std::function<void(const Event&)>& listener)
```
* listener - A callable object that will be called when an event of type `Event` is dispatched. The object must be copyable.

Returns a handle that can be used to remove the listener.

### Removing Listeners

Remove a listener that was previously registered using `listen()`.

```cpp
void remove(std::uint64_t handle)
```
* handle - The handle of the listener to remove.

### Checking Listeners

Check if a listener is registered in the dispatcher.

```cpp
bool hasListener(std::uint64_t handle)
```
* handle - The handle of the listener to check.

### Dispatching Events

Immediately dispatch an event to all the listeners that are registered for the event type.

```cpp
template<typename Event>
void dispatch(const Event& event)
```
* event - The event to dispatch.

### Queueing Events

Add an event to the queue. The event will be dispatched once `process()` is called.

```cpp
template<typename Event>
void queue(const Event& event)
```
* event - The event to queue. The event will be copied and stored in the queue.

### Processing the Queue

Processing the queue will dispatch all the events that were queued using `queue()`. The events will be dispatched in the order they were queued.

```cpp
void process()
```

## Some notes about callback safety

* You can safely call `listen()` and `listenOnce()` from inside a listener callback. The new listener will not be called during the current dispatching process.
* You can safely call `remove()` from inside a listener callback. The listener will be removed immediately and will not be called during the current dispatching process. But keep in mind that the dispatching order is not guaranteed, so the listener may be called before it is removed.
* You can safely call `dispatch()` and `process()` from inside a listener callback. The new event will be dispatched immediately during the current dispatching process.
* You can safely call `remove(handle)` for a handle that was already removed. Nothing will happen.
* Handles are never reused by the same dispatcher.

## Token - helper RAII class
```cpp

tinevents::Dispatcher dispatcher;
std::uint64_t handle = dispatcher.listen<MyEvent>(myCallback);

// RAII token
tinyevents::Token token(dispatcher, handle); // When this token goes out of scope the listener
                                             // will be automatically removed from dispatcher.
```

Make sure that the dispatcher is still alive when the token is destroyed.

## Tests

Tests are written using Google Test. The library is fetched automatically by CMake during the configuration step of the tests.

In order to run the tests with CMake, you can use the following commands:
```
cmake -S tests -B tests/build
cmake --build tests/build
ctest --test-dir tests/build
```

## License
Copyright © 2023-2024 KyrietS\
Use of this software is granted under the terms of the MIT License.

See the [LICENSE](LICENSE) file for more details.
