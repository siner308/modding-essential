cmake_minimum_required(VERSION 3.16)
project(VisualEffects)

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
    D3D11Hook.cpp
    VisualEffects.cpp
)

set(HEADERS
    D3D11Hook.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries (Windows specific)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        d3d11
        dxgi
        d3dcompiler
        kernel32
        user32
        gdi32
    )
    
    # Note: In a real implementation, you would also link:
    # - Microsoft Detours library for hooking
    # - Additional DirectX libraries as needed
    # target_link_libraries(${PROJECT_NAME} detours)
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Copy shader files to output directory (if you have external shader files)
# file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Create a build script for Windows
if(WIN32)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build.bat
        "@echo off\n"
        "echo Building Visual Effects System...\n"
        "mkdir build 2>nul\n"
        "cd build\n"
        "cmake .. -G \"Visual Studio 16 2019\" -A x64\n"
        "cmake --build . --config Release\n"
        "echo.\n"
        "echo Build complete! Executable is in build/bin/Release/\n"
        "echo.\n"
        "echo IMPORTANT: You need to install Microsoft Detours library\n"
        "echo for the hooking functionality to work properly.\n"
        "echo.\n"
        "echo Download from: https://github.com/Microsoft/Detours\n"
        "pause\n"
    )
endif()

# Development notes
message(STATUS "=== Visual Effects System Build Configuration ===")
message(STATUS "This project demonstrates DirectX 11 hooking for visual effects")
message(STATUS "")
message(STATUS "IMPORTANT DEPENDENCIES NEEDED:")
message(STATUS "1. Microsoft Detours - for function hooking")
message(STATUS "2. DirectX 11 SDK - included with Windows SDK")
message(STATUS "3. D3DCompiler - for runtime shader compilation")
message(STATUS "")
message(STATUS "USAGE NOTES:")
message(STATUS "- Run as Administrator for memory access")
message(STATUS "- Target DirectX 11 applications only")
message(STATUS "- Disable anti-virus during development")
message(STATUS "- Use in offline games to avoid anti-cheat issues")
message(STATUS "=============================================")