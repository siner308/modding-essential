cmake_minimum_required(VERSION 3.10)
project(ModFramework)

set(CMAKE_CXX_STANDARD 17)

add_library(ModFramework SHARED
    Main.cpp
    ModLoader.cpp
    EventManager.cpp
)

target_include_directories(ModFramework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
