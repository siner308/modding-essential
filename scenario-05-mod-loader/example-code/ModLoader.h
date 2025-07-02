#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <filesystem>

/**
 * Universal Mod Loader System
 * 
 * This system provides a comprehensive mod loading framework that can:
 * - Load DLL mods dynamically
 * - Provide mod API for consistent mod development
 * - Handle mod dependencies and conflicts
 * - Manage mod configuration and settings
 * - Support hot-reloading for development
 */

// Forward declarations
class ModLoader;
class Mod;

// Mod API version for compatibility checking
#define MOD_API_VERSION 1

// Standard mod interface that all mods must implement
extern "C" {
    // Mod initialization function - called when mod is loaded
    typedef bool(*ModInitFunc)(ModLoader* loader);
    
    // Mod cleanup function - called when mod is unloaded
    typedef void(*ModCleanupFunc)();
    
    // Mod info function - returns mod metadata
    typedef const char*(*ModInfoFunc)();
    
    // Mod API version check
    typedef int(*ModAPIVersionFunc)();
}

// Mod metadata structure
struct ModInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> dependencies;
    std::vector<std::string> conflicts;
    bool isLoaded = false;
    bool isEnabled = false;
};

// Mod class representing a loaded mod
class Mod {
private:
    HMODULE moduleHandle;
    ModInfo info;
    std::filesystem::path modPath;
    
    // Function pointers
    ModInitFunc initFunc;
    ModCleanupFunc cleanupFunc;
    ModInfoFunc infoFunc;
    ModAPIVersionFunc versionFunc;

public:
    Mod(const std::filesystem::path& path);
    ~Mod();
    
    // Loading and unloading
    bool Load();
    void Unload();
    bool Initialize(ModLoader* loader);
    void Cleanup();
    
    // Information
    const ModInfo& GetInfo() const { return info; }
    const std::filesystem::path& GetPath() const { return modPath; }
    bool IsLoaded() const { return moduleHandle != nullptr; }
    bool IsEnabled() const { return info.isEnabled; }
    
    // Control
    void SetEnabled(bool enabled) { info.isEnabled = enabled; }
    
private:
    bool LoadFunctions();
    bool ParseModInfo();
};

// Hook system for mod API
class HookManager {
private:
    struct Hook {
        void* originalFunction;
        void* hookFunction;
        void** originalPointer;
        bool isActive;
        std::string name;
    };
    
    std::vector<Hook> hooks;

public:
    // Install function hook
    bool InstallHook(const std::string& name, void* targetFunction, 
                    void* hookFunction, void** originalFunction);
    
    // Remove function hook
    bool RemoveHook(const std::string& name);
    
    // Remove all hooks
    void RemoveAllHooks();
    
    // Get hook status
    bool IsHookActive(const std::string& name);
    std::vector<std::string> GetActiveHooks();
};

// Configuration manager for mods
class ConfigManager {
private:
    std::map<std::string, std::map<std::string, std::string>> configs;
    std::filesystem::path configPath;

public:
    ConfigManager(const std::filesystem::path& path);
    
    // Configuration file operations
    bool LoadConfig(const std::string& modName);
    bool SaveConfig(const std::string& modName);
    
    // Value operations
    void SetString(const std::string& modName, const std::string& key, const std::string& value);
    void SetInt(const std::string& modName, const std::string& key, int value);
    void SetFloat(const std::string& modName, const std::string& key, float value);
    void SetBool(const std::string& modName, const std::string& key, bool value);
    
    std::string GetString(const std::string& modName, const std::string& key, const std::string& defaultValue = "");
    int GetInt(const std::string& modName, const std::string& key, int defaultValue = 0);
    float GetFloat(const std::string& modName, const std::string& key, float defaultValue = 0.0f);
    bool GetBool(const std::string& modName, const std::string& key, bool defaultValue = false);
    
    // Check if key exists
    bool HasKey(const std::string& modName, const std::string& key);
    
    // Remove key/mod
    void RemoveKey(const std::string& modName, const std::string& key);
    void RemoveModConfig(const std::string& modName);
};

// Event system for mod communication
class EventManager {
public:
    using EventCallback = std::function<void(const std::string&, void*)>;
    
private:
    std::map<std::string, std::vector<EventCallback>> eventHandlers;

public:
    // Event registration
    void RegisterEvent(const std::string& eventName, EventCallback callback);
    void UnregisterEvent(const std::string& eventName, EventCallback callback);
    
    // Event triggering
    void TriggerEvent(const std::string& eventName, void* data = nullptr);
    void TriggerEventDeferred(const std::string& eventName, void* data = nullptr);
    
    // Event processing
    void ProcessDeferredEvents();
    
    // Event information
    bool HasEvent(const std::string& eventName);
    std::vector<std::string> GetRegisteredEvents();
};

// Main mod loader class
class ModLoader {
private:
    std::vector<std::unique_ptr<Mod>> loadedMods;
    std::filesystem::path modsDirectory;
    std::filesystem::path configDirectory;
    
    std::unique_ptr<HookManager> hookManager;
    std::unique_ptr<ConfigManager> configManager;
    std::unique_ptr<EventManager> eventManager;
    
    // Hot reload support
    bool hotReloadEnabled;
    std::map<std::filesystem::path, std::filesystem::file_time_type> fileWatchList;
    
    // Dependency resolution
    std::vector<std::string> loadOrder;
    bool dependenciesResolved;

public:
    ModLoader(const std::filesystem::path& modsDir, const std::filesystem::path& configDir);
    ~ModLoader();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Mod management
    bool LoadMod(const std::filesystem::path& modPath);
    void UnloadMod(const std::string& modName);
    void UnloadAllMods();
    bool ReloadMod(const std::string& modName);
    
    // Mod discovery
    void ScanForMods();
    std::vector<std::filesystem::path> FindModFiles();
    
    // Mod information
    std::vector<ModInfo> GetLoadedMods();
    Mod* FindMod(const std::string& modName);
    bool IsModLoaded(const std::string& modName);
    
    // Mod control
    void EnableMod(const std::string& modName);
    void DisableMod(const std::string& modName);
    void SetModEnabled(const std::string& modName, bool enabled);
    
    // Dependency management
    bool ResolveDependencies();
    std::vector<std::string> GetLoadOrder();
    bool CheckConflicts();
    
    // Hot reload
    void EnableHotReload(bool enable) { hotReloadEnabled = enable; }
    void CheckForModUpdates();
    void ProcessHotReload();
    
    // API access for mods
    HookManager* GetHookManager() { return hookManager.get(); }
    ConfigManager* GetConfigManager() { return configManager.get(); }
    EventManager* GetEventManager() { return eventManager.get(); }
    
    // Utility functions
    void LogMessage(const std::string& message);
    void LogError(const std::string& error);
    void LogWarning(const std::string& warning);
    
    // Safety and validation
    bool ValidateModFile(const std::filesystem::path& modPath);
    bool CheckModSecurity(const std::filesystem::path& modPath);
    
private:
    // Internal helpers
    bool LoadModInternal(std::unique_ptr<Mod> mod);
    void BuildDependencyGraph();
    bool SortByDependencies();
    void InitializeModAPIs();
    
    // File monitoring
    void AddToWatchList(const std::filesystem::path& path);
    void RemoveFromWatchList(const std::filesystem::path& path);
    bool HasFileChanged(const std::filesystem::path& path);
};

// Mod API functions that mods can use
namespace ModAPI {
    // Memory operations
    bool ReadMemory(void* address, void* buffer, size_t size);
    bool WriteMemory(void* address, const void* buffer, size_t size);
    void* AllocateMemory(size_t size);
    void FreeMemory(void* ptr);
    
    // Process operations
    HANDLE GetCurrentProcessHandle();
    DWORD GetCurrentProcessId();
    void* GetModuleBase(const std::string& moduleName);
    void* GetProcAddress(const std::string& moduleName, const std::string& functionName);
    
    // Pattern scanning
    void* FindPattern(const std::string& pattern, const std::string& mask, 
                     void* startAddress = nullptr, size_t searchSize = 0);
    std::vector<void*> FindAllPatterns(const std::string& pattern, const std::string& mask);
    
    // Hook utilities
    bool InstallInlineHook(void* targetFunction, void* hookFunction, void** originalFunction);
    bool InstallVTableHook(void* object, int index, void* hookFunction, void** originalFunction);
    bool RemoveHook(void* hookFunction);
    
    // Logging
    void Log(const std::string& message);
    void LogError(const std::string& error);
    void LogWarning(const std::string& warning);
    void LogDebug(const std::string& debug);
    
    // Configuration
    void SetConfig(const std::string& key, const std::string& value);
    std::string GetConfig(const std::string& key, const std::string& defaultValue = "");
    void SetConfigInt(const std::string& key, int value);
    int GetConfigInt(const std::string& key, int defaultValue = 0);
    void SetConfigFloat(const std::string& key, float value);
    float GetConfigFloat(const std::string& key, float defaultValue = 0.0f);
    void SetConfigBool(const std::string& key, bool value);
    bool GetConfigBool(const std::string& key, bool defaultValue = false);
    
    // Events
    void RegisterEventHandler(const std::string& eventName, EventManager::EventCallback callback);
    void TriggerEvent(const std::string& eventName, void* data = nullptr);
    
    // File operations
    std::string GetModsDirectory();
    std::string GetConfigDirectory();
    bool FileExists(const std::string& path);
    std::vector<std::string> ListFiles(const std::string& directory, const std::string& extension = "");
    
    // Game integration helpers
    void* GetGameWindow();
    bool IsGameForeground();
    void SetGameTitle(const std::string& title);
}

// Utility macros for mod development
#define MOD_EXPORT extern "C" __declspec(dllexport)

// Standard mod entry points
#define IMPLEMENT_MOD(name, version, author, description) \
    MOD_EXPORT int GetModAPIVersion() { return MOD_API_VERSION; } \
    MOD_EXPORT const char* GetModInfo() { \
        static std::string info = std::string(name) + "|" + std::string(version) + "|" + \
                                std::string(author) + "|" + std::string(description); \
        return info.c_str(); \
    }

// Hook installation helper
#define INSTALL_HOOK(name, target, hook, original) \
    ModAPI::InstallInlineHook((void*)target, (void*)hook, (void**)&original)

// Configuration helpers
#define GET_CONFIG_STRING(key, default) ModAPI::GetConfig(key, default)
#define SET_CONFIG_STRING(key, value) ModAPI::SetConfig(key, value)
#define GET_CONFIG_INT(key, default) ModAPI::GetConfigInt(key, default)
#define SET_CONFIG_INT(key, value) ModAPI::SetConfigInt(key, value)
#define GET_CONFIG_BOOL(key, default) ModAPI::GetConfigBool(key, default)
#define SET_CONFIG_BOOL(key, value) ModAPI::SetConfigBool(key, value)

// Logging helpers
#define MOD_LOG(msg) ModAPI::Log(msg)
#define MOD_LOG_ERROR(msg) ModAPI::LogError(msg)
#define MOD_LOG_WARNING(msg) ModAPI::LogWarning(msg)
#define MOD_LOG_DEBUG(msg) ModAPI::LogDebug(msg)