#include "ModLoader.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

void ShowMainMenu() {
    std::cout << "\n=== Universal Mod Loader ===\n";
    std::cout << "1. Load Mod\n";
    std::cout << "2. Unload Mod\n";
    std::cout << "3. List Loaded Mods\n";
    std::cout << "4. Enable/Disable Mod\n";
    std::cout << "5. Reload Mod\n";
    std::cout << "6. Scan for New Mods\n";
    std::cout << "7. Mod Configuration\n";
    std::cout << "8. Hook Manager\n";
    std::cout << "9. Event System\n";
    std::cout << "10. Settings\n";
    std::cout << "0. Exit\n";
    std::cout << "Choice: ";
}

void ListLoadedMods(ModLoader& loader) {
    auto mods = loader.GetLoadedMods();
    
    if (mods.empty()) {
        std::cout << "\nNo mods loaded.\n";
        return;
    }
    
    std::cout << "\n=== Loaded Mods ===\n";
    for (size_t i = 0; i < mods.size(); ++i) {
        const auto& mod = mods[i];
        std::cout << (i + 1) << ". " << mod.name << " v" << mod.version << "\n";
        std::cout << "   Author: " << mod.author << "\n";
        std::cout << "   Description: " << mod.description << "\n";
        std::cout << "   Status: " << (mod.isEnabled ? "Enabled" : "Disabled") << "\n";
        
        if (!mod.dependencies.empty()) {
            std::cout << "   Dependencies: ";
            for (size_t j = 0; j < mod.dependencies.size(); ++j) {
                std::cout << mod.dependencies[j];
                if (j < mod.dependencies.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        if (!mod.conflicts.empty()) {
            std::cout << "   Conflicts: ";
            for (size_t j = 0; j < mod.conflicts.size(); ++j) {
                std::cout << mod.conflicts[j];
                if (j < mod.conflicts.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        std::cout << "\n";
    }
}

void LoadModMenu(ModLoader& loader) {
    std::cout << "\n=== Load Mod ===\n";
    std::cout << "1. Load specific mod file\n";
    std::cout << "2. Scan and load from mods directory\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            std::cout << "Enter mod file path: ";
            std::string modPath;
            std::cin.ignore();
            std::getline(std::cin, modPath);
            
            if (loader.LoadMod(modPath)) {
                std::cout << "Mod loaded successfully!\n";
            } else {
                std::cout << "Failed to load mod.\n";
            }
            break;
        }
        
        case 2: {
            std::cout << "Scanning for mods...\n";
            loader.ScanForMods();
            std::cout << "Scan complete.\n";
            break;
        }
    }
}

void UnloadModMenu(ModLoader& loader) {
    auto mods = loader.GetLoadedMods();
    
    if (mods.empty()) {
        std::cout << "\nNo mods to unload.\n";
        return;
    }
    
    std::cout << "\n=== Unload Mod ===\n";
    for (size_t i = 0; i < mods.size(); ++i) {
        std::cout << (i + 1) << ". " << mods[i].name << "\n";
    }
    std::cout << "0. Cancel\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice > 0 && choice <= static_cast<int>(mods.size())) {
        std::string modName = mods[choice - 1].name;
        loader.UnloadMod(modName);
        std::cout << "Mod unloaded: " << modName << "\n";
    }
}

void ModControlMenu(ModLoader& loader) {
    auto mods = loader.GetLoadedMods();
    
    if (mods.empty()) {
        std::cout << "\nNo mods available.\n";
        return;
    }
    
    std::cout << "\n=== Enable/Disable Mod ===\n";
    for (size_t i = 0; i < mods.size(); ++i) {
        std::cout << (i + 1) << ". " << mods[i].name 
                  << " [" << (mods[i].isEnabled ? "Enabled" : "Disabled") << "]\n";
    }
    std::cout << "0. Cancel\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice > 0 && choice <= static_cast<int>(mods.size())) {
        std::string modName = mods[choice - 1].name;
        bool currentState = mods[choice - 1].isEnabled;
        
        loader.SetModEnabled(modName, !currentState);
        std::cout << "Mod " << modName << " " << (!currentState ? "enabled" : "disabled") << "\n";
    }
}

void ReloadModMenu(ModLoader& loader) {
    auto mods = loader.GetLoadedMods();
    
    if (mods.empty()) {
        std::cout << "\nNo mods to reload.\n";
        return;
    }
    
    std::cout << "\n=== Reload Mod ===\n";
    for (size_t i = 0; i < mods.size(); ++i) {
        std::cout << (i + 1) << ". " << mods[i].name << "\n";
    }
    std::cout << "0. Cancel\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice > 0 && choice <= static_cast<int>(mods.size())) {
        std::string modName = mods[choice - 1].name;
        if (loader.ReloadMod(modName)) {
            std::cout << "Mod reloaded successfully: " << modName << "\n";
        } else {
            std::cout << "Failed to reload mod: " << modName << "\n";
        }
    }
}

void ConfigurationMenu(ModLoader& loader) {
    std::cout << "\n=== Mod Configuration ===\n";
    std::cout << "1. View mod config\n";
    std::cout << "2. Set config value\n";
    std::cout << "3. Save all configs\n";
    std::cout << "4. Reload configs\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            auto mods = loader.GetLoadedMods();
            if (mods.empty()) {
                std::cout << "No mods loaded.\n";
                break;
            }
            
            std::cout << "Select mod:\n";
            for (size_t i = 0; i < mods.size(); ++i) {
                std::cout << (i + 1) << ". " << mods[i].name << "\n";
            }
            
            int modChoice;
            std::cin >> modChoice;
            
            if (modChoice > 0 && modChoice <= static_cast<int>(mods.size())) {
                std::string modName = mods[modChoice - 1].name;
                auto* configManager = loader.GetConfigManager();
                
                // This would show config values (implementation would require 
                // exposing config enumeration in ConfigManager)
                std::cout << "Configuration for " << modName << ":\n";
                std::cout << "(Config viewing not fully implemented in this demo)\n";
            }
            break;
        }
        
        case 2: {
            std::cout << "Enter mod name: ";
            std::string modName;
            std::cin >> modName;
            
            std::cout << "Enter config key: ";
            std::string key;
            std::cin >> key;
            
            std::cout << "Enter config value: ";
            std::string value;
            std::cin.ignore();
            std::getline(std::cin, value);
            
            loader.GetConfigManager()->SetString(modName, key, value);
            std::cout << "Config set: " << modName << "." << key << " = " << value << "\n";
            break;
        }
        
        case 3: {
            auto mods = loader.GetLoadedMods();
            for (const auto& mod : mods) {
                loader.GetConfigManager()->SaveConfig(mod.name);
            }
            std::cout << "All configs saved.\n";
            break;
        }
        
        case 4: {
            auto mods = loader.GetLoadedMods();
            for (const auto& mod : mods) {
                loader.GetConfigManager()->LoadConfig(mod.name);
            }
            std::cout << "All configs reloaded.\n";
            break;
        }
    }
}

void HookManagerMenu(ModLoader& loader) {
    std::cout << "\n=== Hook Manager ===\n";
    
    auto* hookManager = loader.GetHookManager();
    auto activeHooks = hookManager->GetActiveHooks();
    
    std::cout << "Active hooks: " << activeHooks.size() << "\n";
    for (const auto& hook : activeHooks) {
        std::cout << "  - " << hook << "\n";
    }
    
    std::cout << "\n1. Remove hook\n";
    std::cout << "2. Remove all hooks\n";
    std::cout << "3. Check hook status\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            if (activeHooks.empty()) {
                std::cout << "No hooks to remove.\n";
                break;
            }
            
            for (size_t i = 0; i < activeHooks.size(); ++i) {
                std::cout << (i + 1) << ". " << activeHooks[i] << "\n";
            }
            
            int hookChoice;
            std::cin >> hookChoice;
            
            if (hookChoice > 0 && hookChoice <= static_cast<int>(activeHooks.size())) {
                std::string hookName = activeHooks[hookChoice - 1];
                if (hookManager->RemoveHook(hookName)) {
                    std::cout << "Hook removed: " << hookName << "\n";
                } else {
                    std::cout << "Failed to remove hook: " << hookName << "\n";
                }
            }
            break;
        }
        
        case 2: {
            hookManager->RemoveAllHooks();
            std::cout << "All hooks removed.\n";
            break;
        }
        
        case 3: {
            std::cout << "Enter hook name: ";
            std::string hookName;
            std::cin >> hookName;
            
            bool isActive = hookManager->IsHookActive(hookName);
            std::cout << "Hook " << hookName << " is " << (isActive ? "active" : "inactive") << "\n";
            break;
        }
    }
}

void EventSystemMenu(ModLoader& loader) {
    std::cout << "\n=== Event System ===\n";
    
    auto* eventManager = loader.GetEventManager();
    auto events = eventManager->GetRegisteredEvents();
    
    std::cout << "Registered events: " << events.size() << "\n";
    for (const auto& event : events) {
        std::cout << "  - " << event << "\n";
    }
    
    std::cout << "\n1. Trigger event\n";
    std::cout << "2. Test mod communication\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            std::cout << "Enter event name: ";
            std::string eventName;
            std::cin >> eventName;
            
            eventManager->TriggerEvent(eventName, nullptr);
            std::cout << "Event triggered: " << eventName << "\n";
            break;
        }
        
        case 2: {
            // Trigger some test events
            eventManager->TriggerEvent("game_start", nullptr);
            eventManager->TriggerEvent("player_level_up", nullptr);
            eventManager->TriggerEvent("config_changed", nullptr);
            std::cout << "Test events triggered.\n";
            break;
        }
    }
}

void SettingsMenu(ModLoader& loader) {
    std::cout << "\n=== Settings ===\n";
    std::cout << "1. Toggle hot reload\n";
    std::cout << "2. Check for mod updates\n";
    std::cout << "3. Validate all mods\n";
    std::cout << "4. View mod loader info\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: {
            std::cout << "Enable hot reload? (y/n): ";
            char enable;
            std::cin >> enable;
            
            bool hotReload = (enable == 'y' || enable == 'Y');
            loader.EnableHotReload(hotReload);
            std::cout << "Hot reload " << (hotReload ? "enabled" : "disabled") << "\n";
            break;
        }
        
        case 2: {
            std::cout << "Checking for mod updates...\n";
            loader.CheckForModUpdates();
            std::cout << "Update check complete.\n";
            break;
        }
        
        case 3: {
            std::cout << "Validating all mods...\n";
            // This would iterate through all mod files and validate them
            auto modFiles = loader.FindModFiles();
            int validMods = 0;
            
            for (const auto& modFile : modFiles) {
                if (loader.ValidateModFile(modFile)) {
                    validMods++;
                }
            }
            
            std::cout << "Validation complete. " << validMods << "/" << modFiles.size() 
                      << " mods are valid.\n";
            break;
        }
        
        case 4: {
            std::cout << "\n=== Mod Loader Information ===\n";
            std::cout << "API Version: " << MOD_API_VERSION << "\n";
            std::cout << "Loaded Mods: " << loader.GetLoadedMods().size() << "\n";
            
            auto* hookManager = loader.GetHookManager();
            std::cout << "Active Hooks: " << hookManager->GetActiveHooks().size() << "\n";
            
            auto* eventManager = loader.GetEventManager();
            std::cout << "Registered Events: " << eventManager->GetRegisteredEvents().size() << "\n";
            break;
        }
    }
}

int main() {
    std::cout << "=== Universal Game Mod Loader ===\n";
    std::cout << "Advanced mod loading system with API support\n";
    std::cout << "\nFeatures:\n";
    std::cout << "- Dynamic DLL mod loading\n";
    std::cout << "- Dependency resolution\n";
    std::cout << "- Hot reload support\n";
    std::cout << "- Configuration management\n";
    std::cout << "- Hook management\n";
    std::cout << "- Event system\n";
    
    // Get directories
    std::cout << "\nEnter mods directory (or press Enter for './mods'): ";
    std::string modsDir;
    std::getline(std::cin, modsDir);
    if (modsDir.empty()) {
        modsDir = "./mods";
    }
    
    std::cout << "Enter config directory (or press Enter for './config'): ";
    std::string configDir;
    std::getline(std::cin, configDir);
    if (configDir.empty()) {
        configDir = "./config";
    }
    
    // Initialize mod loader
    ModLoader loader(modsDir, configDir);
    
    if (!loader.Initialize()) {
        std::cout << "\nFailed to initialize mod loader!\n";
        std::cout << "Check that directories exist and are accessible.\n";
        return 1;
    }
    
    std::cout << "\nMod loader initialized successfully!\n";
    std::cout << "Mods directory: " << modsDir << "\n";
    std::cout << "Config directory: " << configDir << "\n";
    
    // Main application loop
    bool running = true;
    while (running) {
        ShowMainMenu();
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                LoadModMenu(loader);
                break;
                
            case 2:
                UnloadModMenu(loader);
                break;
                
            case 3:
                ListLoadedMods(loader);
                break;
                
            case 4:
                ModControlMenu(loader);
                break;
                
            case 5:
                ReloadModMenu(loader);
                break;
                
            case 6:
                std::cout << "\nScanning for new mods...\n";
                loader.ScanForMods();
                std::cout << "Scan complete.\n";
                break;
                
            case 7:
                ConfigurationMenu(loader);
                break;
                
            case 8:
                HookManagerMenu(loader);
                break;
                
            case 9:
                EventSystemMenu(loader);
                break;
                
            case 10:
                SettingsMenu(loader);
                break;
                
            case 0:
                running = false;
                break;
                
            default:
                std::cout << "Invalid choice!\n";
                break;
        }
        
        // Small delay and update checks
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check for hot reload updates
        loader.CheckForModUpdates();
    }
    
    // Cleanup
    std::cout << "\nShutting down mod loader...\n";
    loader.Shutdown();
    
    std::cout << "Thank you for using Universal Mod Loader!\n";
    return 0;
}