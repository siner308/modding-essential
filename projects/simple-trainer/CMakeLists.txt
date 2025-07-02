cmake_minimum_required(VERSION 3.10)
project(SimpleTrainer)

set(CMAKE_CXX_STANDARD 17)

add_library(SimpleTrainer SHARED
    Main.cpp
    Trainer.cpp
)

target_include_directories(SimpleTrainer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
