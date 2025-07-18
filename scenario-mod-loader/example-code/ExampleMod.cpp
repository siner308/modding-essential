// ExampleMod.cpp - Example mod implementation using the ModLoader API
#include "ModLoader.h"
#include <iostream>
#include <string>

// Example mod that demonstrates various ModLoader features
// This mod adds a simple FPS counter and hot-key system

// Global variables for the mod
static bool g_showFPS = true;
static bool g_modEnabled = true;
static HWND g_gameWindow = nullptr;
static std::chrono::steady_clock::time_point g_lastFrame;
static float g_currentFPS = 0.0f;
static int g_frameCount = 0;

// Original function pointers (for hooks)
typedef BOOL(WINAPI* SetWindowTextA_t)(HWND hWnd, LPCSTR lpString);
static SetWindowTextA_t oSetWindowTextA = nullptr;

// Hook functions
BOOL WINAPI hkSetWindowTextA(HWND hWnd, LPCSTR lpString) {
    // Intercept window title changes to add FPS info
    if (g_modEnabled && g_showFPS && hWnd == g_gameWindow) {
        std::string newTitle = std::string(lpString) + " [FPS: " + std::to_string((int)g_currentFPS) + "]";
        return oSetWindowTextA(hWnd, newTitle.c_str());
    }
    
    return oSetWindowTextA(hWnd, lpString);
}

// FPS calculation
void UpdateFPS() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - g_lastFrame).count();
    
    if (elapsed >= 1.0f) { // Update every second
        g_currentFPS = g_frameCount / elapsed;
        g_frameCount = 0;
        g_lastFrame = now;
    } else {
        g_frameCount++;
    }
}

// Event handlers
void OnGameStart(const std::string& eventName, void* data) {
    MOD_LOG("Game started - FPS counter activated");
    g_gameWindow = (HWND)ModAPI::GetGameWindow();
}

void OnGameEnd(const std::string& eventName, void* data) {
    MOD_LOG("Game ended - FPS counter deactivated");
    g_gameWindow = nullptr;
}

void OnConfigChanged(const std::string& eventName, void* data) {
    // Reload configuration when config file changes
    g_showFPS = GET_CONFIG_BOOL("show_fps", true);
    MOD_LOG("Configuration reloaded");
}

// Hotkey handler (called from main game loop)
void ProcessHotkeys() {
    // F10 - Toggle FPS display
    if (GetAsyncKeyState(VK_F10) & 0x8000) {
        static bool f10Pressed = false;
        if (!f10Pressed) {
            g_showFPS = !g_showFPS;
            SET_CONFIG_BOOL("show_fps", g_showFPS);
            
            std::string status = g_showFPS ? "enabled" : "disabled";
            MOD_LOG("FPS display " + status);
            
            f10Pressed = true;
        }
    } else {
        static bool f10Pressed = false;
        f10Pressed = false;
    }
    
    // F11 - Toggle mod entirely
    if (GetAsyncKeyState(VK_F11) & 0x8000) {
        static bool f11Pressed = false;
        if (!f11Pressed) {
            g_modEnabled = !g_modEnabled;
            SET_CONFIG_BOOL("mod_enabled", g_modEnabled);
            
            std::string status = g_modEnabled ? "enabled" : "disabled";
            MOD_LOG("Example mod " + status);
            
            f11Pressed = true;
        }
    } else {
        static bool f11Pressed = false;
        f11Pressed = false;
    }
}

// Main mod update function (called every frame)
void UpdateMod() {
    if (!g_modEnabled) return;
    
    UpdateFPS();
    ProcessHotkeys();
}

// Required mod export functions
MOD_EXPORT int GetModAPIVersion() {
    return MOD_API_VERSION;
}

MOD_EXPORT const char* GetModInfo() {
    return "ExampleMod|1.0.0|ModLoader Team|Example mod demonstrating ModLoader features";
}

MOD_EXPORT bool ModInit(ModLoader* loader) {
    MOD_LOG("Initializing Example Mod v1.0.0");
    
    // Load configuration
    g_showFPS = GET_CONFIG_BOOL("show_fps", true);
    g_modEnabled = GET_CONFIG_BOOL("mod_enabled", true);
    
    // Register event handlers
    ModAPI::RegisterEventHandler("game_start", OnGameStart);
    ModAPI::RegisterEventHandler("game_end", OnGameEnd);
    ModAPI::RegisterEventHandler("config_changed", OnConfigChanged);
    
    // Install hooks
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) {
        void* setWindowTextA = GetProcAddress(user32, "SetWindowTextA");
        if (setWindowTextA) {
            if (INSTALL_HOOK("SetWindowTextA", setWindowTextA, hkSetWindowTextA, oSetWindowTextA)) {
                MOD_LOG("Successfully hooked SetWindowTextA");
            } else {
                MOD_LOG_ERROR("Failed to hook SetWindowTextA");
            }
        }
    }
    
    // Initialize FPS timer
    g_lastFrame = std::chrono::steady_clock::now();
    
    // Set up update callback (in a real implementation, this would be done differently)
    // For this example, we'll assume the game calls UpdateMod() each frame
    
    MOD_LOG("Example Mod initialized successfully");
    MOD_LOG("Controls:");
    MOD_LOG("  F10 - Toggle FPS display");
    MOD_LOG("  F11 - Toggle mod on/off");
    
    return true;
}

MOD_EXPORT void ModCleanup() {
    MOD_LOG("Cleaning up Example Mod");
    
    // Save configuration
    SET_CONFIG_BOOL("show_fps", g_showFPS);
    SET_CONFIG_BOOL("mod_enabled", g_modEnabled);
    
    // Remove hooks would be done automatically by ModLoader
    // but we could do manual cleanup here if needed
    
    MOD_LOG("Example Mod cleanup complete");
}

// Additional example functions demonstrating advanced features

// Example of inter-mod communication
void SendMessageToOtherMods() {
    struct ModMessage {
        std::string senderName = "ExampleMod";
        std::string messageType = "fps_update";
        float fpsValue = g_currentFPS;
    };
    
    ModMessage msg;
    ModAPI::TriggerEvent("fps_updated", &msg);
}

// Example of memory patching
bool PatchGameMemory() {
    // Find a pattern in game memory
    void* address = ModAPI::FindPattern("48 8B 05 ? ? ? ? 48 85 C0", "xxx????xxx");
    if (address) {
        // Read current value
        float currentValue;
        if (ModAPI::ReadMemory(address, &currentValue, sizeof(float))) {
            MOD_LOG("Current value at address: " + std::to_string(currentValue));
            
            // Modify value
            float newValue = currentValue * 1.5f;
            if (ModAPI::WriteMemory(address, &newValue, sizeof(float))) {
                MOD_LOG("Successfully patched memory");
                return true;
            }
        }
    }
    
    MOD_LOG_ERROR("Failed to patch memory");
    return false;
}

// Example of file operations
void SaveModData() {
    std::string dataPath = ModAPI::GetConfigDirectory() + "/ExampleMod_data.txt";
    std::ofstream file(dataPath);
    if (file.is_open()) {
        file << "FPS History:\n";
        file << "Current FPS: " << g_currentFPS << "\n";
        file << "Mod Enabled: " << (g_modEnabled ? "Yes" : "No") << "\n";
        file << "Show FPS: " << (g_showFPS ? "Yes" : "No") << "\n";
        file.close();
        
        MOD_LOG("Data saved to " + dataPath);
    }
}

void LoadModData() {
    std::string dataPath = ModAPI::GetConfigDirectory() + "/ExampleMod_data.txt";
    if (ModAPI::FileExists(dataPath)) {
        std::ifstream file(dataPath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                MOD_LOG("Loaded data: " + line);
            }
            file.close();
        }
    }
}

// Example of advanced hooking - VTable hook
bool HookDirectXPresent() {
    // This would be used to hook DirectX Present function for rendering overlays
    void* d3dDevice = nullptr; // Would get actual D3D device
    
    if (d3dDevice) {
        // Hook VTable function (Present is usually at index 17 for D3D11)
        void* presentFunction = nullptr; // Would get from VTable
        
        // Example hook installation
        return INSTALL_HOOK("D3D_Present", presentFunction, nullptr, nullptr);
    }
    
    return false;
}

// Example mod configuration structure
struct ExampleModConfig {
    bool showFPS = true;
    bool modEnabled = true;
    float fpsUpdateInterval = 1.0f;
    std::string displayFormat = "FPS: {0}";
    int maxFPSHistory = 100;
    bool logFPSToFile = false;
};

// Save/load complex configuration
void SaveComplexConfig() {
    ExampleModConfig config;
    config.showFPS = g_showFPS;
    config.modEnabled = g_modEnabled;
    
    // In a real implementation, you might use JSON or XML
    SET_CONFIG_BOOL("show_fps", config.showFPS);
    SET_CONFIG_BOOL("mod_enabled", config.modEnabled);
    SET_CONFIG_FLOAT("fps_update_interval", config.fpsUpdateInterval);
    SET_CONFIG_STRING("display_format", config.displayFormat);
    SET_CONFIG_INT("max_fps_history", config.maxFPSHistory);
    SET_CONFIG_BOOL("log_fps_to_file", config.logFPSToFile);
}

ExampleModConfig LoadComplexConfig() {
    ExampleModConfig config;
    
    config.showFPS = GET_CONFIG_BOOL("show_fps", true);
    config.modEnabled = GET_CONFIG_BOOL("mod_enabled", true);
    config.fpsUpdateInterval = GET_CONFIG_FLOAT("fps_update_interval", 1.0f);
    config.displayFormat = GET_CONFIG_STRING("display_format", "FPS: {0}");
    config.maxFPSHistory = GET_CONFIG_INT("max_fps_history", 100);
    config.logFPSToFile = GET_CONFIG_BOOL("log_fps_to_file", false);
    
    return config;
}