#include "FPSUnlocker.h"
#include <chrono>
#include <thread>
#include <algorithm>

// FPS Presets
const float AdvancedFPSController::FPS_PRESETS[] = {30.0f, 60.0f, 120.0f, 144.0f, 240.0f, 0.0f}; // 0.0f = unlimited
const int AdvancedFPSController::PRESET_COUNT = sizeof(FPS_PRESETS) / sizeof(float);

FPSUnlocker::FPSUnlocker() : processHandle(nullptr), processId(0), 
                             fpsAddress(0), originalFPS(60.0f), isUnlocked(false) {
}

FPSUnlocker::~FPSUnlocker() {
    if (isUnlocked) {
        RestoreFPS();
    }
    if (processHandle) {
        CloseHandle(processHandle);
    }
}

bool FPSUnlocker::Initialize(const std::wstring& processName) {
    // Find target process
    processId = GetProcessIdByName(processName);
    if (processId == 0) {
        std::wcout << L"Process not found: " << processName << std::endl;
        return false;
    }

    // Validate it's a game process
    if (!IsValidGameProcess(processId)) {
        std::cout << "Invalid or protected process" << std::endl;
        return false;
    }

    // Open process with required permissions
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!processHandle) {
        std::cout << "Failed to open process. Run as administrator." << std::endl;
        return false;
    }

    std::wcout << L"Successfully attached to: " << processName << L" (PID: " << processId << L")" << std::endl;
    return true;
}

DWORD FPSUnlocker::GetProcessIdByName(const std::wstring& processName) {
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    DWORD foundPid = 0;
    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                foundPid = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe32));
    }
    
    CloseHandle(snapshot);
    return foundPid;
}

bool FPSUnlocker::IsValidGameProcess(DWORD pid) {
    // Basic validation - check if process has a window
    HWND hwnd = nullptr;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        DWORD windowPid;
        GetWindowThreadProcessId(hwnd, &windowPid);
        if (windowPid == *(DWORD*)lParam && IsWindowVisible(hwnd)) {
            *(HWND*)((DWORD*)lParam + 1) = hwnd;
            return FALSE;
        }
        return TRUE;
    }, (LPARAM)&pid);
    
    return hwnd != nullptr;
}

std::vector<uintptr_t> FPSUnlocker::ScanForFloat(float value) {
    std::vector<uintptr_t> results;
    
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    
    std::cout << "Scanning for FPS value: " << value << std::endl;
    
    while (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        // Only scan committed memory that's readable/writable
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            
            std::vector<char> buffer(mbi.RegionSize);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, mbi.BaseAddress, 
                                buffer.data(), mbi.RegionSize, &bytesRead)) {
                
                // Search for float value (allowing small floating point errors)
                for (size_t i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
                    float* floatPtr = reinterpret_cast<float*>(&buffer[i]);
                    if (abs(*floatPtr - value) < 0.001f) {
                        uintptr_t foundAddress = (uintptr_t)mbi.BaseAddress + i;
                        results.push_back(foundAddress);
                        
                        if (results.size() % 100 == 0) {
                            std::cout << "Found " << results.size() << " potential addresses..." << std::endl;
                        }
                    }
                }
            }
        }
        address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    
    std::cout << "Total addresses found: " << results.size() << std::endl;
    return results;
}

bool FPSUnlocker::FindFPSLimit() {
    std::cout << "Searching for FPS limit..." << std::endl;
    
    // First scan for common FPS values
    std::vector<float> commonFPS = {60.0f, 30.0f, 120.0f, 144.0f};
    std::vector<uintptr_t> allAddresses;
    
    for (float fps : commonFPS) {
        auto addresses = ScanForFloat(fps);
        allAddresses.insert(allAddresses.end(), addresses.begin(), addresses.end());
    }
    
    if (allAddresses.empty()) {
        std::cout << "No FPS values found. Game might use different storage method." << std::endl;
        return false;
    }
    
    // Validate each address by testing modification
    std::cout << "Validating " << allAddresses.size() << " addresses..." << std::endl;
    
    for (auto addr : allAddresses) {
        if (ValidateAddress(addr)) {
            fpsAddress = addr;
            originalFPS = ReadFloat(addr);
            std::cout << "FPS address found: 0x" << std::hex << addr << std::dec;
            std::cout << " (Current value: " << originalFPS << ")" << std::endl;
            return true;
        }
    }
    
    std::cout << "No valid FPS address found." << std::endl;
    return false;
}

bool FPSUnlocker::ValidateAddress(uintptr_t address) {
    // Read current value
    float currentValue = ReadFloat(address);
    if (currentValue < 10.0f || currentValue > 1000.0f) {
        return false; // Unreasonable FPS value
    }
    
    // Test write access
    float testValue = currentValue + 1.0f;
    if (!WriteFloat(address, testValue)) {
        return false;
    }
    
    // Small delay to let the change take effect
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Verify the change
    float readBack = ReadFloat(address);
    bool isValid = (abs(readBack - testValue) < 0.1f);
    
    // Restore original value
    WriteFloat(address, currentValue);
    
    return isValid;
}

bool FPSUnlocker::SetFPS(float targetFPS) {
    if (fpsAddress == 0) {
        std::cout << "FPS address not found. Call FindFPSLimit() first." << std::endl;
        return false;
    }
    
    // Validate FPS range
    if (targetFPS != 0.0f && (targetFPS < 10.0f || targetFPS > 1000.0f)) {
        std::cout << "Invalid FPS value: " << targetFPS << std::endl;
        return false;
    }
    
    // For unlimited FPS, use a very high value
    float actualFPS = (targetFPS == 0.0f) ? 9999.0f : targetFPS;
    
    if (WriteFloat(fpsAddress, actualFPS)) {
        isUnlocked = true;
        std::cout << "FPS set to: " << (targetFPS == 0.0f ? "Unlimited" : std::to_string((int)targetFPS)) << std::endl;
        return true;
    }
    
    std::cout << "Failed to set FPS" << std::endl;
    return false;
}

bool FPSUnlocker::RestoreFPS() {
    if (fpsAddress == 0 || !isUnlocked) {
        return false;
    }
    
    if (WriteFloat(fpsAddress, originalFPS)) {
        isUnlocked = false;
        std::cout << "FPS restored to original: " << originalFPS << std::endl;
        return true;
    }
    
    return false;
}

float FPSUnlocker::GetCurrentFPS() {
    if (fpsAddress == 0) return 0.0f;
    return ReadFloat(fpsAddress);
}

float FPSUnlocker::ReadFloat(uintptr_t address) {
    float value = 0.0f;
    SIZE_T bytesRead;
    ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(float), &bytesRead);
    return value;
}

bool FPSUnlocker::WriteFloat(uintptr_t address, float value) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(float), &bytesWritten);
}

// Advanced FPS Controller Implementation
AdvancedFPSController::AdvancedFPSController(FPSUnlocker* fpsUnlocker) 
    : unlocker(fpsUnlocker), currentFPS(60.0f), targetFPS(60.0f), 
      hotkeyEnabled(false), messageWindow(nullptr), currentPresetIndex(1),
      isTransitioning(false), transitionDuration(0.5f) {
    CreateMessageWindow();
}

AdvancedFPSController::~AdvancedFPSController() {
    DisableHotkeys();
    DestroyMessageWindow();
}

bool AdvancedFPSController::EnableHotkeys() {
    if (!messageWindow) return false;
    
    // Register hotkeys
    RegisterHotKey(messageWindow, 1, 0, VK_F1); // F1: Increase FPS
    RegisterHotKey(messageWindow, 2, 0, VK_F2); // F2: Decrease FPS
    RegisterHotKey(messageWindow, 3, MOD_CONTROL, VK_F1); // Ctrl+F1: Next preset
    RegisterHotKey(messageWindow, 4, MOD_CONTROL, VK_F2); // Ctrl+F2: Previous preset
    RegisterHotKey(messageWindow, 5, 0, VK_F3); // F3: Restore original
    
    hotkeyEnabled = true;
    std::cout << "Hotkeys enabled:" << std::endl;
    std::cout << "  F1: Increase FPS (+10)" << std::endl;
    std::cout << "  F2: Decrease FPS (-10)" << std::endl;
    std::cout << "  Ctrl+F1: Next preset" << std::endl;
    std::cout << "  Ctrl+F2: Previous preset" << std::endl;
    std::cout << "  F3: Restore original FPS" << std::endl;
    
    return true;
}

void AdvancedFPSController::DisableHotkeys() {
    if (!hotkeyEnabled || !messageWindow) return;
    
    for (int i = 1; i <= 5; i++) {
        UnregisterHotKey(messageWindow, i);
    }
    
    hotkeyEnabled = false;
    std::cout << "Hotkeys disabled" << std::endl;
}

void AdvancedFPSController::ProcessMessages() {
    if (!messageWindow) return;
    
    MSG msg;
    while (PeekMessage(&msg, messageWindow, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
                case 1: // F1 - Increase FPS
                    targetFPS = std::min(targetFPS + 10.0f, 300.0f);
                    SetFPSSmooth(targetFPS);
                    break;
                    
                case 2: // F2 - Decrease FPS
                    targetFPS = std::max(targetFPS - 10.0f, 30.0f);
                    SetFPSSmooth(targetFPS);
                    break;
                    
                case 3: // Ctrl+F1 - Next preset
                    NextPreset();
                    break;
                    
                case 4: // Ctrl+F2 - Previous preset  
                    PreviousPreset();
                    break;
                    
                case 5: // F3 - Restore
                    unlocker->RestoreFPS();
                    targetFPS = currentFPS = unlocker->GetCurrentFPS();
                    break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void AdvancedFPSController::NextPreset() {
    currentPresetIndex = (currentPresetIndex + 1) % PRESET_COUNT;
    float fps = FPS_PRESETS[currentPresetIndex];
    SetFPSSmooth(fps);
    
    std::cout << "Preset " << (currentPresetIndex + 1) << ": ";
    if (fps == 0.0f) {
        std::cout << "Unlimited FPS" << std::endl;
    } else {
        std::cout << fps << " FPS" << std::endl;
    }
}

void AdvancedFPSController::PreviousPreset() {
    currentPresetIndex = (currentPresetIndex - 1 + PRESET_COUNT) % PRESET_COUNT;
    float fps = FPS_PRESETS[currentPresetIndex];
    SetFPSSmooth(fps);
    
    std::cout << "Preset " << (currentPresetIndex + 1) << ": ";
    if (fps == 0.0f) {
        std::cout << "Unlimited FPS" << std::endl;
    } else {
        std::cout << fps << " FPS" << std::endl;
    }
}

void AdvancedFPSController::SetFPSSmooth(float fps, float duration) {
    if (!unlocker || !unlocker->IsInitialized()) return;
    
    startFPS = currentFPS;
    targetFPS = fps;
    transitionDuration = duration;
    transitionStart = std::chrono::steady_clock::now();
    isTransitioning = true;
}

void AdvancedFPSController::Update() {
    if (!isTransitioning) return;
    
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - transitionStart).count();
    
    if (elapsed >= transitionDuration) {
        currentFPS = targetFPS;
        unlocker->SetFPS(currentFPS);
        isTransitioning = false;
    } else {
        float t = elapsed / transitionDuration;
        currentFPS = LerpFPS(startFPS, targetFPS, t);
        unlocker->SetFPS(currentFPS);
    }
}

float AdvancedFPSController::LerpFPS(float from, float to, float t) {
    // Smooth step interpolation
    t = t * t * (3.0f - 2.0f * t);
    return from + (to - from) * t;
}

void AdvancedFPSController::CreateMessageWindow() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"FPSControllerWindow";
    RegisterClass(&wc);
    
    messageWindow = CreateWindow(
        L"FPSControllerWindow", L"FPS Controller",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), this
    );
}

void AdvancedFPSController::DestroyMessageWindow() {
    if (messageWindow) {
        DestroyWindow(messageWindow);
        messageWindow = nullptr;
    }
}

LRESULT CALLBACK AdvancedFPSController::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// FPS Utils Implementation
namespace FPSUtils {
    FPSMonitor::FPSMonitor(size_t sampleCount) : maxSamples(sampleCount) {
        frameTimes.reserve(maxSamples);
        lastFrame = std::chrono::steady_clock::now();
    }
    
    void FPSMonitor::RecordFrame() {
        auto now = std::chrono::steady_clock::now();
        float frameTime = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;
        
        if (frameTime > 0.0f) {
            frameTimes.push_back(frameTime);
            if (frameTimes.size() > maxSamples) {
                frameTimes.erase(frameTimes.begin());
            }
        }
    }
    
    float FPSMonitor::GetAverageFPS() {
        if (frameTimes.empty()) return 0.0f;
        
        float total = 0.0f;
        for (float time : frameTimes) {
            total += time;
        }
        return frameTimes.size() / total;
    }
    
    float FPSMonitor::GetMinFPS() {
        if (frameTimes.empty()) return 0.0f;
        
        float maxTime = *std::max_element(frameTimes.begin(), frameTimes.end());
        return 1.0f / maxTime;
    }
    
    float FPSMonitor::GetMaxFPS() {
        if (frameTimes.empty()) return 0.0f;
        
        float minTime = *std::min_element(frameTimes.begin(), frameTimes.end());
        return 1.0f / minTime;
    }
    
    void FPSMonitor::Reset() {
        frameTimes.clear();
        lastFrame = std::chrono::steady_clock::now();
    }
    
    bool IsGameFPSChangeSafe(const std::wstring& processName) {
        // List of games known to work well with FPS changes
        std::vector<std::wstring> safeGames = {
            L"eldenring.exe",
            L"DarkSoulsIII.exe",
            L"skyrimse.exe",
            L"witcher3.exe"
        };
        
        std::wstring lowerName = processName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        for (const auto& safe : safeGames) {
            std::wstring lowerSafe = safe;
            std::transform(lowerSafe.begin(), lowerSafe.end(), lowerSafe.begin(), ::tolower);
            if (lowerName.find(lowerSafe) != std::wstring::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    float GetRecommendedMaxFPS(const std::wstring& processName) {
        std::wstring lowerName = processName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find(L"eldenring") != std::wstring::npos ||
            lowerName.find(L"darksouls") != std::wstring::npos) {
            return 120.0f; // FromSoft games physics can break above 120
        }
        
        if (lowerName.find(L"skyrim") != std::wstring::npos) {
            return 144.0f; // Skyrim SE handles up to 144 well
        }
        
        return 240.0f; // Default safe maximum
    }
    
    bool IsValidFPSValue(float fps) {
        return (fps == 0.0f) || (fps >= 15.0f && fps <= 1000.0f);
    }
}