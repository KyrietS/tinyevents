cmake_minimum_required(VERSION 3.11)
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)

set(TEST_TARGET tinyevents_tests)

project(${TEST_TARGET})
set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TEST_TARGET})

add_executable(${TEST_TARGET}
        TestEventDispatch.cpp
        TestEventQueue.cpp
        TestEventDispatcherMove.cpp
        TestEventListen.cpp
)

# tinyevents
add_subdirectory(../ tinyevents)
target_link_libraries(${TEST_TARGET} PRIVATE tinyevents)

# GoogleTest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
target_link_libraries(${TEST_TARGET} PRIVATE gtest gtest_main gmock)

enable_testing()
include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
