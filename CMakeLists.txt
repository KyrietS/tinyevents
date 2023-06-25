cmake_minimum_required(VERSION 3.11)

project(tinyevents)

set(CMAKE_CXX_STANDARD 17)
add_library(tinyevents INTERFACE include/tinyevents/tinyevents.hpp)
target_include_directories(tinyevents INTERFACE include)
