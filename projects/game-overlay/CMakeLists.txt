cmake_minimum_required(VERSION 3.10)
project(GameOverlay)

set(CMAKE_CXX_STANDARD 17)

# Add ImGui source files
file(GLOB IMGUI_SOURCES "vendor/imgui/*.cpp")

add_library(GameOverlay SHARED
    main.cpp
    GameOverlay.cpp
    D3D11Hook.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(GameOverlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
find_library(D3D11_LIBRARY d3d11.lib)
target_link_libraries(GameOverlay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor/minhook/lib/minhook.x64.lib ${D3D11_LIBRARY})
