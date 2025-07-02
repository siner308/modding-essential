cmake_minimum_required(VERSION 3.16)
project(FPSUnlocker)

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
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(SOURCES
    main.cpp
    FPSUnlocker.cpp
)

set(HEADERS
    FPSUnlocker.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries (Windows specific)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        kernel32
        user32
        psapi
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

# Create a simple build script for Windows
if(WIN32)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build.bat
        "@echo off\n"
        "echo Building FPS Unlocker...\n"
        "mkdir build 2>nul\n"
        "cd build\n"
        "cmake .. -G \"Visual Studio 16 2019\" -A x64\n"
        "cmake --build . --config Release\n"
        "echo.\n"
        "echo Build complete! Executable is in build/bin/Release/\n"
        "pause\n"
    )
endif()