cmake_minimum_required(VERSION 3.16)
project(ModLoader)

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

# Main ModLoader executable
set(MAIN_SOURCES
    main.cpp
    ModLoader.cpp
)

set(HEADERS
    ModLoader.h
)

# Create main executable
add_executable(${PROJECT_NAME} ${MAIN_SOURCES} ${HEADERS})

# Example mod DLL
set(MOD_SOURCES
    ExampleMod.cpp
)

# Create example mod as DLL
add_library(ExampleMod SHARED ${MOD_SOURCES} ${HEADERS})

# Link libraries (Windows specific)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        kernel32
        user32
        psapi
        advapi32
    )
    
    target_link_libraries(ExampleMod
        kernel32
        user32
    )
endif()

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(ExampleMod PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/mods
    OUTPUT_NAME "ExampleMod"
)

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(TARGETS ExampleMod
    RUNTIME DESTINATION mods
)

# Create directories and sample files
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/mods)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/config)

# Create example mod configuration file
file(WRITE ${CMAKE_BINARY_DIR}/config/ExampleMod.ini
    "show_fps=true\n"
    "mod_enabled=true\n"
    "fps_update_interval=1.0\n"
    "display_format=FPS: {0}\n"
    "max_fps_history=100\n"
    "log_fps_to_file=false\n"
)

# Create mod information file for ExampleMod
file(WRITE ${CMAKE_BINARY_DIR}/mods/ExampleMod.ini
    "[ModInfo]\n"
    "name=ExampleMod\n"
    "version=1.0.0\n"
    "author=ModLoader Team\n"
    "description=Example mod demonstrating ModLoader features\n"
    "dependencies=\n"
    "conflicts=\n"
    "\n"
    "[Settings]\n"
    "auto_load=true\n"
    "priority=normal\n"
)

# Create build script for Windows
if(WIN32)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/build.bat
        "@echo off\n"
        "echo Building Universal Mod Loader...\n"
        "mkdir build 2>nul\n"
        "cd build\n"
        "cmake .. -G \"Visual Studio 16 2019\" -A x64\n"
        "cmake --build . --config Release\n"
        "echo.\n"
        "echo Build complete! Files are in build/bin/ and build/mods/\n"
        "echo.\n"
        "echo Directory structure:\n"
        "echo   build/bin/Release/ModLoader.exe  - Main application\n"
        "echo   build/mods/Release/ExampleMod.dll - Example mod\n"
        "echo   build/config/ExampleMod.ini - Example mod config\n"
        "echo.\n"
        "echo IMPORTANT: Run ModLoader.exe from the build directory\n"
        "echo so it can find the mods and config folders.\n"
        "pause\n"
    )
endif()

# Create development helper scripts
if(WIN32)
    # Create mod development template
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/create_mod_template.bat
        "@echo off\n"
        "set /p modname=Enter mod name: \n"
        "echo Creating mod template for %modname%...\n"
        "\n"
        "mkdir mod_templates\\%modname% 2>nul\n"
        "\n"
        "echo // %modname%.cpp - Generated mod template> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo #include \"../ModLoader.h\">> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo.>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo MOD_EXPORT int GetModAPIVersion() {>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     return MOD_API_VERSION;>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo }>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo.>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo MOD_EXPORT const char* GetModInfo() {>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     return \"%modname%^|1.0.0^|YourName^|Mod description here\";>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo }>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo.>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo MOD_EXPORT bool ModInit(ModLoader* loader) {>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     MOD_LOG(\"Initializing %modname%\");>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     // Add your initialization code here>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     return true;>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo }>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo.>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo MOD_EXPORT void ModCleanup() {>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     MOD_LOG(\"Cleaning up %modname%\");>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo     // Add your cleanup code here>> mod_templates\\%modname%\\%modname%.cpp\n"
        "echo }>> mod_templates\\%modname%\\%modname%.cpp\n"
        "\n"
        "echo Template created in mod_templates\\%modname%\\\n"
        "echo Copy ModLoader.h to your mod directory and compile as DLL\n"
        "pause\n"
    )
endif()

# Development notes
message(STATUS "=== Universal Mod Loader Build Configuration ===")
message(STATUS "This project provides a comprehensive mod loading framework")
message(STATUS "")
message(STATUS "COMPONENTS:")
message(STATUS "- ModLoader.exe: Main mod loader application")
message(STATUS "- ExampleMod.dll: Example mod demonstrating the API")
message(STATUS "- Configuration system with INI files")
message(STATUS "- Hook management system")
message(STATUS "- Event system for mod communication")
message(STATUS "")
message(STATUS "FEATURES:")
message(STATUS "- Dynamic DLL loading and unloading")
message(STATUS "- Dependency resolution")
message(STATUS "- Hot reload support for development")
message(STATUS "- Configuration management")
message(STATUS "- Memory patching and hooking support")
message(STATUS "- Inter-mod communication via events")
message(STATUS "")
message(STATUS "DIRECTORY STRUCTURE:")
message(STATUS "- /bin/: Main executable")
message(STATUS "- /mods/: Mod DLL files")
message(STATUS "- /config/: Mod configuration files")
message(STATUS "")
message(STATUS "DEVELOPMENT:")
message(STATUS "1. Build the project")
message(STATUS "2. Run ModLoader.exe from build directory") 
message(STATUS "3. Use provided templates to create new mods")
message(STATUS "4. Place mod DLLs in mods/ directory")
message(STATUS "5. Configure mods via config/ INI files")
message(STATUS "")
message(STATUS "MOD API USAGE:")
message(STATUS "- Include ModLoader.h in your mod project")
message(STATUS "- Implement required export functions")
message(STATUS "- Use MOD_LOG, INSTALL_HOOK, GET_CONFIG macros")
message(STATUS "- Compile as DLL and place in mods/ directory")
message(STATUS "=============================================")