cmake_minimum_required(VERSION 3.16)
project(BangaloreMetro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

# Backend server executable
add_executable(metro_server
    backend/main.cpp
)

target_include_directories(metro_server PRIVATE backend/)

# Link pthreads for multi-threaded connection handling
find_package(Threads REQUIRED)
target_link_libraries(metro_server PRIVATE Threads::Threads)

# Build messages
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output: metro_server")
