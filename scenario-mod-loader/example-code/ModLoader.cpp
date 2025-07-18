#include "ModLoader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

// Global mod loader instance for API access
static ModLoader* g_ModLoader = nullptr;

// Mod class implementation
Mod::Mod(const std::filesystem::path& path) 
    : modPath(path), moduleHandle(nullptr), initFunc(nullptr), 
      cleanupFunc(nullptr), infoFunc(nullptr), versionFunc(nullptr) {
}

Mod::~Mod() {
    if (IsLoaded()) {
        Unload();
    }
}

bool Mod::Load() {
    if (IsLoaded()) {
        return true; // Already loaded
    }
    
    std::cout << "Loading mod: " << modPath.filename().string() << std::endl;
    
    moduleHandle = LoadLibraryW(modPath.wstring().c_str());
    if (!moduleHandle) {
        std::cout << "Failed to load mod library. Error: " << GetLastError() << std::endl;
        return false;
    }
    
    if (!LoadFunctions()) {
        std::cout << "Failed to load mod functions" << std::endl;
        Unload();
        return false;
    }
    
    // Check API version compatibility
    if (versionFunc && versionFunc() != MOD_API_VERSION) {
        std::cout << "Mod API version mismatch. Expected: " << MOD_API_VERSION 
                  << ", Got: " << versionFunc() << std::endl;
        Unload();
        return false;
    }
    
    if (!ParseModInfo()) {
        std::cout << "Failed to parse mod information" << std::endl;
        Unload();
        return false;
    }
    
    info.isLoaded = true;
    std::cout << "Successfully loaded mod: " << info.name << " v" << info.version << std::endl;
    return true;
}

void Mod::Unload() {
    if (!IsLoaded()) {
        return;
    }
    
    std::cout << "Unloading mod: " << info.name << std::endl;
    
    if (info.isEnabled) {
        Cleanup();
    }
    
    FreeLibrary(moduleHandle);
    moduleHandle = nullptr;
    info.isLoaded = false;
    info.isEnabled = false;
}

bool Mod::Initialize(ModLoader* loader) {
    if (!IsLoaded() || !initFunc) {
        return false;
    }
    
    std::cout << "Initializing mod: " << info.name << std::endl;
    
    if (initFunc(loader)) {
        info.isEnabled = true;
        std::cout << "Mod initialized successfully: " << info.name << std::endl;
        return true;
    } else {
        std::cout << "Failed to initialize mod: " << info.name << std::endl;
        return false;
    }
}

void Mod::Cleanup() {
    if (cleanupFunc && info.isEnabled) {
        std::cout << "Cleaning up mod: " << info.name << std::endl;
        cleanupFunc();
        info.isEnabled = false;
    }
}

bool Mod::LoadFunctions() {
    // Load required functions
    initFunc = (ModInitFunc)GetProcAddress(moduleHandle, "ModInit");
    cleanupFunc = (ModCleanupFunc)GetProcAddress(moduleHandle, "ModCleanup");
    infoFunc = (ModInfoFunc)GetProcAddress(moduleHandle, "GetModInfo");
    versionFunc = (ModAPIVersionFunc)GetProcAddress(moduleHandle, "GetModAPIVersion");
    
    // initFunc and infoFunc are required
    return (initFunc != nullptr && infoFunc != nullptr);
}

bool Mod::ParseModInfo() {
    if (!infoFunc) {
        return false;
    }
    
    const char* infoString = infoFunc();
    if (!infoString) {
        return false;
    }
    
    // Parse info string format: "name|version|author|description"
    std::string infoStr(infoString);
    std::vector<std::string> parts;
    std::stringstream ss(infoStr);
    std::string part;
    
    while (std::getline(ss, part, '|')) {
        parts.push_back(part);
    }
    
    if (parts.size() >= 4) {
        info.name = parts[0];
        info.version = parts[1];
        info.author = parts[2];
        info.description = parts[3];
        
        // Try to load additional info from config file
        std::filesystem::path configPath = modPath.parent_path() / (info.name + ".ini");
        if (std::filesystem::exists(configPath)) {
            std::ifstream configFile(configPath);
            std::string line;
            
            while (std::getline(configFile, line)) {
                if (line.find("dependencies=") == 0) {
                    std::string deps = line.substr(13);
                    std::stringstream depStream(deps);
                    std::string dep;
                    while (std::getline(depStream, dep, ',')) {
                        info.dependencies.push_back(dep);
                    }
                } else if (line.find("conflicts=") == 0) {
                    std::string conflicts = line.substr(10);
                    std::stringstream confStream(conflicts);
                    std::string conf;
                    while (std::getline(confStream, conf, ',')) {
                        info.conflicts.push_back(conf);
                    }
                }
            }
        }
        
        return true;
    }
    
    return false;
}

// HookManager implementation
bool HookManager::InstallHook(const std::string& name, void* targetFunction, 
                             void* hookFunction, void** originalFunction) {
    // Remove existing hook with same name
    RemoveHook(name);
    
    Hook hook;
    hook.name = name;
    hook.originalFunction = targetFunction;
    hook.hookFunction = hookFunction;
    hook.originalPointer = originalFunction;
    hook.isActive = false;
    
    // In a real implementation, this would use a hooking library like Microsoft Detours
    // For this example, we'll simulate the hook installation
    std::cout << "Installing hook: " << name << std::endl;
    
    // Simulate successful hook installation
    if (originalFunction) {
        *originalFunction = targetFunction;
    }
    hook.isActive = true;
    
    hooks.push_back(hook);
    return true;
}

bool HookManager::RemoveHook(const std::string& name) {
    auto it = std::find_if(hooks.begin(), hooks.end(), 
                          [&name](const Hook& h) { return h.name == name; });
    
    if (it != hooks.end()) {
        std::cout << "Removing hook: " << name << std::endl;
        
        // In a real implementation, this would restore the original function
        it->isActive = false;
        hooks.erase(it);
        return true;
    }
    
    return false;
}

void HookManager::RemoveAllHooks() {
    std::cout << "Removing all hooks (" << hooks.size() << ")" << std::endl;
    
    for (auto& hook : hooks) {
        hook.isActive = false;
    }
    hooks.clear();
}

bool HookManager::IsHookActive(const std::string& name) {
    auto it = std::find_if(hooks.begin(), hooks.end(), 
                          [&name](const Hook& h) { return h.name == name; });
    return it != hooks.end() && it->isActive;
}

std::vector<std::string> HookManager::GetActiveHooks() {
    std::vector<std::string> activeHooks;
    for (const auto& hook : hooks) {
        if (hook.isActive) {
            activeHooks.push_back(hook.name);
        }
    }
    return activeHooks;
}

// ConfigManager implementation
ConfigManager::ConfigManager(const std::filesystem::path& path) : configPath(path) {
    if (!std::filesystem::exists(configPath)) {
        std::filesystem::create_directories(configPath);
    }
}

bool ConfigManager::LoadConfig(const std::string& modName) {
    std::filesystem::path configFile = configPath / (modName + ".ini");
    if (!std::filesystem::exists(configFile)) {
        return true; // No config file is not an error
    }
    
    std::ifstream file(configFile);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            configs[modName][key] = value;
        }
    }
    
    return true;
}

bool ConfigManager::SaveConfig(const std::string& modName) {
    std::filesystem::path configFile = configPath / (modName + ".ini");
    std::ofstream file(configFile);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : configs[modName]) {
        file << pair.first << "=" << pair.second << std::endl;
    }
    
    return true;
}

void ConfigManager::SetString(const std::string& modName, const std::string& key, const std::string& value) {
    configs[modName][key] = value;
}

void ConfigManager::SetInt(const std::string& modName, const std::string& key, int value) {
    configs[modName][key] = std::to_string(value);
}

void ConfigManager::SetFloat(const std::string& modName, const std::string& key, float value) {
    configs[modName][key] = std::to_string(value);
}

void ConfigManager::SetBool(const std::string& modName, const std::string& key, bool value) {
    configs[modName][key] = value ? "true" : "false";
}

std::string ConfigManager::GetString(const std::string& modName, const std::string& key, const std::string& defaultValue) {
    auto modIt = configs.find(modName);
    if (modIt != configs.end()) {
        auto keyIt = modIt->second.find(key);
        if (keyIt != modIt->second.end()) {
            return keyIt->second;
        }
    }
    return defaultValue;
}

int ConfigManager::GetInt(const std::string& modName, const std::string& key, int defaultValue) {
    std::string value = GetString(modName, key);
    if (!value.empty()) {
        try {
            return std::stoi(value);
        } catch (const std::exception&) {
            // Invalid conversion, return default
        }
    }
    return defaultValue;
}

float ConfigManager::GetFloat(const std::string& modName, const std::string& key, float defaultValue) {
    std::string value = GetString(modName, key);
    if (!value.empty()) {
        try {
            return std::stof(value);
        } catch (const std::exception&) {
            // Invalid conversion, return default
        }
    }
    return defaultValue;
}

bool ConfigManager::GetBool(const std::string& modName, const std::string& key, bool defaultValue) {
    std::string value = GetString(modName, key);
    if (!value.empty()) {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return value == "true" || value == "1" || value == "yes";
    }
    return defaultValue;
}

bool ConfigManager::HasKey(const std::string& modName, const std::string& key) {
    auto modIt = configs.find(modName);
    if (modIt != configs.end()) {
        return modIt->second.find(key) != modIt->second.end();
    }
    return false;
}

void ConfigManager::RemoveKey(const std::string& modName, const std::string& key) {
    auto modIt = configs.find(modName);
    if (modIt != configs.end()) {
        modIt->second.erase(key);
    }
}

void ConfigManager::RemoveModConfig(const std::string& modName) {
    configs.erase(modName);
}

// EventManager implementation
void EventManager::RegisterEvent(const std::string& eventName, EventCallback callback) {
    eventHandlers[eventName].push_back(callback);
}

void EventManager::TriggerEvent(const std::string& eventName, void* data) {
    auto it = eventHandlers.find(eventName);
    if (it != eventHandlers.end()) {
        for (auto& callback : it->second) {
            try {
                callback(eventName, data);
            } catch (const std::exception& e) {
                std::cout << "Exception in event handler for " << eventName << ": " << e.what() << std::endl;
            }
        }
    }
}

bool EventManager::HasEvent(const std::string& eventName) {
    return eventHandlers.find(eventName) != eventHandlers.end();
}

std::vector<std::string> EventManager::GetRegisteredEvents() {
    std::vector<std::string> events;
    for (const auto& pair : eventHandlers) {
        events.push_back(pair.first);
    }
    return events;
}

// ModLoader implementation
ModLoader::ModLoader(const std::filesystem::path& modsDir, const std::filesystem::path& configDir)
    : modsDirectory(modsDir), configDirectory(configDir), 
      hotReloadEnabled(false), dependenciesResolved(false) {
    
    // Set global instance
    g_ModLoader = this;
    
    // Create directories if they don't exist
    if (!std::filesystem::exists(modsDirectory)) {
        std::filesystem::create_directories(modsDirectory);
    }
    if (!std::filesystem::exists(configDirectory)) {
        std::filesystem::create_directories(configDirectory);
    }
    
    // Initialize managers
    hookManager = std::make_unique<HookManager>();
    configManager = std::make_unique<ConfigManager>(configDirectory);
    eventManager = std::make_unique<EventManager>();
}

ModLoader::~ModLoader() {
    Shutdown();
    g_ModLoader = nullptr;
}

bool ModLoader::Initialize() {
    std::cout << "Initializing Mod Loader..." << std::endl;
    std::cout << "Mods directory: " << modsDirectory.string() << std::endl;
    std::cout << "Config directory: " << configDirectory.string() << std::endl;
    
    // Scan for mods
    ScanForMods();
    
    // Resolve dependencies
    if (!ResolveDependencies()) {
        std::cout << "Warning: Some mod dependencies could not be resolved" << std::endl;
    }
    
    std::cout << "Mod Loader initialized successfully" << std::endl;
    return true;
}

void ModLoader::Shutdown() {
    std::cout << "Shutting down Mod Loader..." << std::endl;
    
    // Unload all mods in reverse order
    UnloadAllMods();
    
    // Clear managers
    if (hookManager) {
        hookManager->RemoveAllHooks();
    }
    
    std::cout << "Mod Loader shut down" << std::endl;
}

void ModLoader::ScanForMods() {
    std::cout << "Scanning for mods..." << std::endl;
    
    auto modFiles = FindModFiles();
    std::cout << "Found " << modFiles.size() << " mod files" << std::endl;
    
    for (const auto& modFile : modFiles) {
        LoadMod(modFile);
    }
}

std::vector<std::filesystem::path> ModLoader::FindModFiles() {
    std::vector<std::filesystem::path> modFiles;
    
    if (!std::filesystem::exists(modsDirectory)) {
        return modFiles;
    }
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(modsDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dll") {
            if (ValidateModFile(entry.path())) {
                modFiles.push_back(entry.path());
            }
        }
    }
    
    return modFiles;
}

bool ModLoader::LoadMod(const std::filesystem::path& modPath) {
    // Check if mod is already loaded
    for (const auto& mod : loadedMods) {
        if (mod->GetPath() == modPath) {
            std::cout << "Mod already loaded: " << modPath.filename().string() << std::endl;
            return true;
        }
    }
    
    auto mod = std::make_unique<Mod>(modPath);
    if (!mod->Load()) {
        return false;
    }
    
    // Load mod configuration
    configManager->LoadConfig(mod->GetInfo().name);
    
    // Add to loaded mods
    return LoadModInternal(std::move(mod));
}

bool ModLoader::LoadModInternal(std::unique_ptr<Mod> mod) {
    std::cout << "Loading mod: " << mod->GetInfo().name << std::endl;
    
    // Check conflicts
    for (const auto& conflict : mod->GetInfo().conflicts) {
        if (IsModLoaded(conflict)) {
            std::cout << "Conflict detected: " << mod->GetInfo().name 
                      << " conflicts with " << conflict << std::endl;
            return false;
        }
    }
    
    // Check dependencies
    for (const auto& dependency : mod->GetInfo().dependencies) {
        if (!IsModLoaded(dependency)) {
            std::cout << "Dependency not met: " << mod->GetInfo().name 
                      << " requires " << dependency << std::endl;
            return false;
        }
    }
    
    // Initialize mod
    if (!mod->Initialize(this)) {
        return false;
    }
    
    // Add to watch list for hot reload
    if (hotReloadEnabled) {
        AddToWatchList(mod->GetPath());
    }
    
    // Trigger mod loaded event
    eventManager->TriggerEvent("mod_loaded", (void*)mod.get());
    
    loadedMods.push_back(std::move(mod));
    return true;
}

void ModLoader::UnloadMod(const std::string& modName) {
    auto it = std::find_if(loadedMods.begin(), loadedMods.end(),
                          [&modName](const std::unique_ptr<Mod>& mod) {
                              return mod->GetInfo().name == modName;
                          });
    
    if (it != loadedMods.end()) {
        std::cout << "Unloading mod: " << modName << std::endl;
        
        // Save configuration
        configManager->SaveConfig(modName);
        
        // Remove from watch list
        RemoveFromWatchList((*it)->GetPath());
        
        // Trigger mod unloaded event
        eventManager->TriggerEvent("mod_unloaded", (void*)it->get());
        
        // Unload the mod
        (*it)->Unload();
        loadedMods.erase(it);
    }
}

void ModLoader::UnloadAllMods() {
    // Unload in reverse order to respect dependencies
    while (!loadedMods.empty()) {
        UnloadMod(loadedMods.back()->GetInfo().name);
    }
}

bool ModLoader::ReloadMod(const std::string& modName) {
    auto mod = FindMod(modName);
    if (!mod) {
        return false;
    }
    
    std::filesystem::path modPath = mod->GetPath();
    UnloadMod(modName);
    return LoadMod(modPath);
}

std::vector<ModInfo> ModLoader::GetLoadedMods() {
    std::vector<ModInfo> modInfos;
    for (const auto& mod : loadedMods) {
        modInfos.push_back(mod->GetInfo());
    }
    return modInfos;
}

Mod* ModLoader::FindMod(const std::string& modName) {
    auto it = std::find_if(loadedMods.begin(), loadedMods.end(),
                          [&modName](const std::unique_ptr<Mod>& mod) {
                              return mod->GetInfo().name == modName;
                          });
    
    return it != loadedMods.end() ? it->get() : nullptr;
}

bool ModLoader::IsModLoaded(const std::string& modName) {
    return FindMod(modName) != nullptr;
}

void ModLoader::EnableMod(const std::string& modName) {
    SetModEnabled(modName, true);
}

void ModLoader::DisableMod(const std::string& modName) {
    SetModEnabled(modName, false);
}

void ModLoader::SetModEnabled(const std::string& modName, bool enabled) {
    auto mod = FindMod(modName);
    if (mod) {
        if (enabled && !mod->IsEnabled()) {
            mod->Initialize(this);
        } else if (!enabled && mod->IsEnabled()) {
            mod->Cleanup();
        }
        mod->SetEnabled(enabled);
    }
}

bool ModLoader::ValidateModFile(const std::filesystem::path& modPath) {
    // Basic validation - check if it's a valid DLL
    if (!std::filesystem::exists(modPath)) {
        return false;
    }
    
    // Check file size (avoid extremely small or large files)
    auto fileSize = std::filesystem::file_size(modPath);
    if (fileSize < 1024 || fileSize > 100 * 1024 * 1024) { // 1KB to 100MB
        return false;
    }
    
    // Try to load the library temporarily to check if it's valid
    HMODULE testHandle = LoadLibraryW(modPath.wstring().c_str());
    if (testHandle) {
        FreeLibrary(testHandle);
        return true;
    }
    
    return false;
}

bool ModLoader::CheckModSecurity(const std::filesystem::path& modPath) {
    // In a real implementation, this would:
    // 1. Check digital signatures
    // 2. Scan for known malicious patterns
    // 3. Validate against a whitelist
    // 4. Check file reputation
    
    // For this example, we'll just do basic checks
    return ValidateModFile(modPath);
}

void ModLoader::AddToWatchList(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        fileWatchList[path] = std::filesystem::last_write_time(path);
    }
}

void ModLoader::RemoveFromWatchList(const std::filesystem::path& path) {
    fileWatchList.erase(path);
}

bool ModLoader::HasFileChanged(const std::filesystem::path& path) {
    auto it = fileWatchList.find(path);
    if (it != fileWatchList.end()) {
        if (std::filesystem::exists(path)) {
            auto currentTime = std::filesystem::last_write_time(path);
            return currentTime != it->second;
        }
    }
    return false;
}

void ModLoader::CheckForModUpdates() {
    if (!hotReloadEnabled) return;
    
    for (const auto& pair : fileWatchList) {
        if (HasFileChanged(pair.first)) {
            std::cout << "Mod file changed: " << pair.first.filename().string() << std::endl;
            
            // Find and reload the mod
            for (const auto& mod : loadedMods) {
                if (mod->GetPath() == pair.first) {
                    ReloadMod(mod->GetInfo().name);
                    break;
                }
            }
            
            // Update watch time
            fileWatchList[pair.first] = std::filesystem::last_write_time(pair.first);
        }
    }
}

void ModLoader::LogMessage(const std::string& message) {
    std::cout << "[ModLoader] " << message << std::endl;
}

void ModLoader::LogError(const std::string& error) {
    std::cerr << "[ModLoader ERROR] " << error << std::endl;
}

void ModLoader::LogWarning(const std::string& warning) {
    std::cout << "[ModLoader WARNING] " << warning << std::endl;
}