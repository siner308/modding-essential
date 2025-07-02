cmake_minimum_required(VERSION 3.16)
project(CameraSystem)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # Enable Windows 10 SDK features
    add_compile_definitions(WINVER=0x0A00 _WIN32_WINNT=0x0A00)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(SOURCES
    main.cpp
    CameraSystem.cpp
)

set(HEADERS
    CameraSystem.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries (Windows specific)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        kernel32
        user32
        psapi
        # DirectX Math is header-only, no linking required
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Create build script for Windows
if(WIN32)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build.bat
        "@echo off\n"
        "echo Building Advanced Camera System...\n"
        "mkdir build 2>nul\n"
        "cd build\n"
        "cmake .. -G \"Visual Studio 16 2019\" -A x64\n"
        "cmake --build . --config Release\n"
        "echo.\n"
        "echo Build complete! Executable is in build/bin/Release/\n"
        "echo.\n"
        "echo IMPORTANT: This tool requires:\n"
        "echo 1. Administrator privileges\n"
        "echo 2. Compatible game running\n"
        "echo 3. DirectX Math library (included with Windows SDK)\n"
        "pause\n"
    )
endif()

# Development notes
message(STATUS "=== Advanced Camera System Build Configuration ===")
message(STATUS "This project provides comprehensive camera control for games")
message(STATUS "")
message(STATUS "FEATURES:")
message(STATUS "- Free camera movement with WASD + mouse")
message(STATUS "- Smooth camera transitions and animations")
message(STATUS "- FOV adjustment with safety limits")
message(STATUS "- Cinematic camera sequences")
message(STATUS "- Photo mode with enhanced controls")
message(STATUS "- Camera tracking and follow modes")
message(STATUS "")
message(STATUS "REQUIREMENTS:")
message(STATUS "- Windows 10/11 with latest Windows SDK")
message(STATUS "- DirectX Math library (header-only)")
message(STATUS "- Administrator privileges for memory access")
message(STATUS "- Compatible game engine (Unreal/Unity/Custom)")
message(STATUS "")
message(STATUS "SUPPORTED GAMES:")
message(STATUS "- Elden Ring, Dark Souls series")
message(STATUS "- Skyrim, Fallout series") 
message(STATUS "- The Witcher 3")
message(STATUS "- Most Unreal Engine 4/5 games")
message(STATUS "- Many Unity engine games")
message(STATUS "")
message(STATUS "USAGE:")
message(STATUS "- Run as Administrator")
message(STATUS "- Start target game first")
message(STATUS "- Launch camera system")
message(STATUS "- Use menu to control camera")
message(STATUS "=============================================")