#pragma once
#include <Windows.h>
#include <vector>
#include <iostream>
#include <TlHelp32.h>

/**
 * FPS Unlocker for Games
 * 
 * This class provides functionality to find and modify FPS limit values
 * in running game processes. It scans memory for common FPS values (60.0f)
 * and allows dynamic modification.
 * 
 * Usage:
 *   FPSUnlocker unlocker;
 *   unlocker.Initialize(L"eldenring.exe");
 *   unlocker.FindFPSLimit();
 *   unlocker.SetFPS(120.0f);
 */
class FPSUnlocker {
private:
    HANDLE processHandle;
    DWORD processId;
    uintptr_t fpsAddress;
    float originalFPS;
    bool isUnlocked;

public:
    FPSUnlocker();
    ~FPSUnlocker();
    
    // Main functionality
    bool Initialize(const std::wstring& processName);
    bool FindFPSLimit();
    bool SetFPS(float targetFPS);
    bool RestoreFPS();
    float GetCurrentFPS();
    
    // Status
    bool IsInitialized() const { return processHandle != nullptr; }
    bool IsUnlocked() const { return isUnlocked; }
    uintptr_t GetFPSAddress() const { return fpsAddress; }

private:
    // Memory scanning
    std::vector<uintptr_t> ScanForFloat(float value);
    bool ValidateAddress(uintptr_t address);
    
    // Memory operations
    bool WriteFloat(uintptr_t address, float value);
    float ReadFloat(uintptr_t address);
    
    // Process utilities
    DWORD GetProcessIdByName(const std::wstring& processName);
    bool IsValidGameProcess(DWORD pid);
};

/**
 * Advanced FPS Controller with hotkey support
 * 
 * Extends basic FPS unlocking with:
 * - Hotkey registration (F1/F2 for increase/decrease)
 * - Smooth FPS transitions
 * - Multiple FPS presets
 * - Real-time monitoring
 */
class AdvancedFPSController {
private:
    FPSUnlocker* unlocker;
    float currentFPS;
    float targetFPS;
    bool hotkeyEnabled;
    HWND messageWindow;

    // Preset FPS values
    static const float FPS_PRESETS[];
    static const int PRESET_COUNT;
    int currentPresetIndex;

public:
    AdvancedFPSController(FPSUnlocker* fpsUnlocker);
    ~AdvancedFPSController();
    
    // Hotkey management
    bool EnableHotkeys();
    void DisableHotkeys();
    void ProcessMessages();
    
    // FPS presets
    void NextPreset();
    void PreviousPreset();
    void SetPreset(int index);
    std::vector<float> GetAvailablePresets();
    
    // Smooth transitions
    void SetFPSSmooth(float fps, float duration = 0.5f);
    void Update(); // Call in main loop for smooth transitions

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CreateMessageWindow();
    void DestroyMessageWindow();
    
    // Smooth transition
    float LerpFPS(float from, float to, float t);
    std::chrono::steady_clock::time_point transitionStart;
    float transitionDuration;
    float startFPS;
    bool isTransitioning;
};

// Utility functions for FPS monitoring
namespace FPSUtils {
    /**
     * Calculate actual FPS from frame times
     */
    class FPSMonitor {
    private:
        std::vector<float> frameTimes;
        std::chrono::steady_clock::time_point lastFrame;
        size_t maxSamples;

    public:
        FPSMonitor(size_t sampleCount = 60);
        void RecordFrame();
        float GetAverageFPS();
        float GetMinFPS();
        float GetMaxFPS();
        void Reset();
    };

    /**
     * Detect if game supports dynamic FPS changes
     */
    bool IsGameFPSChangeSafe(const std::wstring& processName);
    
    /**
     * Get recommended FPS for specific games
     */
    float GetRecommendedMaxFPS(const std::wstring& processName);
    
    /**
     * Check if FPS value is within safe range
     */
    bool IsValidFPSValue(float fps);
}