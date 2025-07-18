#include "FPSUnlocker.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

void ShowMenu() {
    std::cout << "\n=== FPS Unlocker Main Menu ===" << std::endl;
    std::cout << "1. Set FPS to 120" << std::endl;
    std::cout << "2. Set FPS to 144" << std::endl;
    std::cout << "3. Set FPS to 240" << std::endl;
    std::cout << "4. Set FPS to Unlimited" << std::endl;
    std::cout << "5. Custom FPS value" << std::endl;
    std::cout << "6. Show current FPS" << std::endl;
    std::cout << "7. Restore original FPS" << std::endl;
    std::cout << "8. Enable hotkey mode" << std::endl;
    std::cout << "9. Exit" << std::endl;
    std::cout << "Choice: ";
}

void RunHotkeyMode(FPSUnlocker& unlocker) {
    AdvancedFPSController controller(&unlocker);
    
    if (!controller.EnableHotkeys()) {
        std::cout << "Failed to enable hotkeys" << std::endl;
        return;
    }
    
    std::cout << "\nHotkey mode enabled. Press Ctrl+C to exit." << std::endl;
    std::cout << "Available hotkeys:" << std::endl;
    std::cout << "  F1/F2: Increase/Decrease FPS by 10" << std::endl;
    std::cout << "  Ctrl+F1/F2: Cycle through presets" << std::endl;
    std::cout << "  F3: Restore original FPS" << std::endl;
    
    // Main message loop
    while (true) {
        controller.ProcessMessages();
        controller.Update(); // For smooth transitions
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS update rate
        
        // Check for exit condition (you might want to add a proper exit mechanism)
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "\nExiting hotkey mode..." << std::endl;
            break;
        }
    }
}

void MonitorFPS(FPSUnlocker& unlocker) {
    FPSUtils::FPSMonitor monitor;
    
    std::cout << "\nMonitoring FPS for 10 seconds..." << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() < 10.0f) {
        monitor.RecordFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "FPS Statistics:" << std::endl;
    std::cout << "  Average: " << monitor.GetAverageFPS() << " FPS" << std::endl;
    std::cout << "  Minimum: " << monitor.GetMinFPS() << " FPS" << std::endl;
    std::cout << "  Maximum: " << monitor.GetMaxFPS() << " FPS" << std::endl;
}

int main() {
    std::cout << "=== Game FPS Unlocker ===" << std::endl;
    std::cout << "Supports: Elden Ring, Dark Souls, Skyrim, and more" << std::endl;
    std::cout << "\nWarning: Use only in offline mode to avoid bans!" << std::endl;
    
    // Get target process name
    std::wstring processName;
    std::cout << "\nEnter game executable name (e.g., eldenring.exe): ";
    std::wcin >> processName;
    
    // Initialize FPS unlocker
    FPSUnlocker unlocker;
    
    if (!unlocker.Initialize(processName)) {
        std::cout << "Failed to initialize. Make sure:" << std::endl;
        std::cout << "1. Game is running" << std::endl;
        std::cout << "2. Running as administrator" << std::endl;
        std::cout << "3. Game is not protected by anti-cheat" << std::endl;
        return 1;
    }
    
    // Check if game is safe for FPS changes
    if (!FPSUtils::IsGameFPSChangeSafe(processName)) {
        std::cout << "\nWarning: This game may not work well with FPS changes." << std::endl;
        std::cout << "Recommended max FPS: " << FPSUtils::GetRecommendedMaxFPS(processName) << std::endl;
        std::cout << "Continue anyway? (y/n): ";
        char choice;
        std::cin >> choice;
        if (choice != 'y' && choice != 'Y') {
            return 0;
        }
    }
    
    // Find FPS limit
    std::cout << "\nSearching for FPS limit in game memory..." << std::endl;
    if (!unlocker.FindFPSLimit()) {
        std::cout << "Could not find FPS limit. This game might:" << std::endl;
        std::cout << "1. Use a different FPS storage method" << std::endl;
        std::cout << "2. Not have an adjustable FPS limit" << std::endl;
        std::cout << "3. Use VSync instead of software limiting" << std::endl;
        return 1;
    }
    
    std::cout << "FPS limit found successfully!" << std::endl;
    std::cout << "Current FPS: " << unlocker.GetCurrentFPS() << std::endl;
    
    // Main menu loop
    while (true) {
        ShowMenu();
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1:
                unlocker.SetFPS(120.0f);
                break;
                
            case 2:
                unlocker.SetFPS(144.0f);
                break;
                
            case 3:
                unlocker.SetFPS(240.0f);
                break;
                
            case 4:
                unlocker.SetFPS(0.0f); // Unlimited
                break;
                
            case 5: {
                float customFPS;
                std::cout << "Enter custom FPS value (0 for unlimited): ";
                std::cin >> customFPS;
                
                if (FPSUtils::IsValidFPSValue(customFPS)) {
                    unlocker.SetFPS(customFPS);
                } else {
                    std::cout << "Invalid FPS value!" << std::endl;
                }
                break;
            }
            
            case 6:
                std::cout << "Current FPS limit: " << unlocker.GetCurrentFPS() << std::endl;
                std::cout << "Status: " << (unlocker.IsUnlocked() ? "Modified" : "Original") << std::endl;
                MonitorFPS(unlocker);
                break;
                
            case 7:
                unlocker.RestoreFPS();
                break;
                
            case 8:
                RunHotkeyMode(unlocker);
                break;
                
            case 9:
                std::cout << "Exiting..." << std::endl;
                return 0;
                
            default:
                std::cout << "Invalid choice!" << std::endl;
                break;
        }
    }
    
    return 0;
}