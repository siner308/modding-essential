#include "CameraSystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <psapi.h>
#include <tlhelp32.h>

// Camera pattern definitions
namespace CameraUtils {
    namespace Patterns {
        // Unreal Engine 4 camera structure pattern
        const std::vector<uint8_t> UE4_CAMERA_PATTERN = {
            0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00,  // mov rax, [rip+offset]
            0x48, 0x85, 0xC0,                           // test rax, rax  
            0x74, 0x00,                                 // jz short
            0xF3, 0x0F, 0x10, 0x40, 0x00               // movss xmm0, [rax+offset]
        };
        const std::vector<bool> UE4_CAMERA_MASK = {
            1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0
        };
        
        // Unity engine camera pattern  
        const std::vector<uint8_t> UNITY_CAMERA_PATTERN = {
            0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov rcx, [rip+offset]
            0x48, 0x85, 0xC9,                           // test rcx, rcx
            0x74, 0x00,                                 // jz short
            0xF3, 0x0F, 0x10, 0x81, 0x00, 0x00, 0x00, 0x00  // movss xmm0, [rcx+offset]
        };
        const std::vector<bool> UNITY_CAMERA_MASK = {
            1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0
        };
        
        // Elden Ring specific camera pattern
        const std::vector<uint8_t> ELDENRING_CAMERA_PATTERN = {
            0x48, 0x8B, 0x15, 0x00, 0x00, 0x00, 0x00,  // mov rdx, [rip+offset]
            0x48, 0x85, 0xD2,                           // test rdx, rdx
            0x74, 0x00,                                 // jz short  
            0xF3, 0x0F, 0x10, 0x42, 0x00               // movss xmm0, [rdx+offset]
        };
        const std::vector<bool> ELDENRING_CAMERA_MASK = {
            1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0
        };
    }
}

// CameraSystem Implementation
CameraSystem::CameraSystem() 
    : processHandle(nullptr), cameraBaseAddress(0), fovAddress(0),
      freeCameraEnabled(false), originalCameraBackup(false),
      safetyMode(true), maxSpeed(100.0f), maxFOV(120.0f), minFOV(30.0f),
      deltaTime(0.0f) {
    
    // Initialize input state
    memset(&input, 0, sizeof(input));
    
    // Initialize camera state
    currentState = CameraState();
    originalState = CameraState();
    
    // Initialize velocity
    freeCameraVelocity = {0.0f, 0.0f, 0.0f};
    
    lastUpdate = std::chrono::steady_clock::now();
}

CameraSystem::~CameraSystem() {
    Shutdown();
}

bool CameraSystem::Initialize(const std::wstring& targetProcess) {
    processName = targetProcess;
    
    // Get process handle
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    
    if (processId == 0) {
        std::wcout << L"Process not found: " << processName << std::endl;
        return false;
    }
    
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!processHandle) {
        std::cout << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }
    
    std::wcout << L"Successfully attached to process: " << processName << std::endl;
    
    // Find camera addresses
    if (!FindCameraBaseAddress()) {
        std::cout << "Failed to find camera base address" << std::endl;
        return false;
    }
    
    if (!FindFOVAddress()) {
        std::cout << "Failed to find FOV address" << std::endl;
        return false;
    }
    
    // Backup original camera state
    if (!GetCameraState(originalState)) {
        std::cout << "Failed to backup original camera state" << std::endl;
        return false;
    }
    
    std::cout << "Camera system initialized successfully" << std::endl;
    return true;
}

void CameraSystem::Shutdown() {
    if (processHandle) {
        if (freeCameraEnabled) {
            EnableFreeCamera(false);
        }
        
        // Restore original camera state
        RestoreOriginalCamera();
        
        CloseHandle(processHandle);
        processHandle = nullptr;
    }
    
    std::cout << "Camera system shut down" << std::endl;
}

bool CameraSystem::FindCameraBaseAddress() {
    std::cout << "Searching for camera base address..." << std::endl;
    
    // Try different patterns based on game engine
    std::vector<std::pair<std::vector<uint8_t>, std::vector<bool>>> patterns = {
        {CameraUtils::Patterns::UE4_CAMERA_PATTERN, CameraUtils::Patterns::UE4_CAMERA_MASK},
        {CameraUtils::Patterns::UNITY_CAMERA_PATTERN, CameraUtils::Patterns::UNITY_CAMERA_MASK},
        {CameraUtils::Patterns::ELDENRING_CAMERA_PATTERN, CameraUtils::Patterns::ELDENRING_CAMERA_MASK}
    };
    
    for (const auto& pattern : patterns) {
        auto addresses = ScanMemoryPattern(pattern.first, pattern.second);
        
        for (uintptr_t addr : addresses) {
            // Validate address by checking if it points to reasonable values
            XMFLOAT3 testPos;
            if (ReadMemory(addr, &testPos, sizeof(testPos))) {
                // Check if position values are reasonable (not too extreme)
                if (abs(testPos.x) < 100000.0f && abs(testPos.y) < 100000.0f && abs(testPos.z) < 100000.0f) {
                    cameraBaseAddress = addr;
                    std::cout << "Found camera base address: 0x" << std::hex << addr << std::endl;
                    return true;
                }
            }
        }
    }
    
    std::cout << "Camera base address not found" << std::endl;
    return false;
}

bool CameraSystem::FindFOVAddress() {
    std::cout << "Searching for FOV address..." << std::endl;
    
    // Common FOV values to scan for
    std::vector<float> commonFOVs = {60.0f, 70.0f, 80.0f, 90.0f, 75.0f, 65.0f};
    
    for (float fov : commonFOVs) {
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t address = 0;
        
        while (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                SIZE_T bytesRead;
                std::vector<uint8_t> buffer(mbi.RegionSize);
                
                if (ReadProcessMemory(processHandle, mbi.BaseAddress, buffer.data(), 
                                    mbi.RegionSize, &bytesRead)) {
                    
                    for (size_t i = 0; i < bytesRead - sizeof(float); i += 4) {
                        float* floatPtr = reinterpret_cast<float*>(&buffer[i]);
                        if (abs(*floatPtr - fov) < 0.1f) {
                            uintptr_t candidateAddr = (uintptr_t)mbi.BaseAddress + i;
                            
                            // Validate by checking nearby memory for camera-related data
                            if (ValidateFOVAddress(candidateAddr)) {
                                fovAddress = candidateAddr;
                                std::cout << "Found FOV address: 0x" << std::hex << candidateAddr << std::endl;
                                return true;
                            }
                        }
                    }
                }
            }
            
            address += mbi.RegionSize;
            if (address == 0) break; // Overflow protection
        }
    }
    
    std::cout << "FOV address not found" << std::endl;
    return false;
}

bool CameraSystem::ValidateFOVAddress(uintptr_t address) {
    float currentFOV;
    if (!ReadValue(address, currentFOV)) return false;
    
    // Check if FOV is in reasonable range
    if (currentFOV < 10.0f || currentFOV > 180.0f) return false;
    
    // Try to modify and restore to confirm it's writable
    float testFOV = currentFOV + 1.0f;
    if (!WriteValue(address, testFOV)) return false;
    
    float readBack;
    if (!ReadValue(address, readBack)) return false;
    
    // Restore original value
    WriteValue(address, currentFOV);
    
    return (abs(readBack - testFOV) < 0.1f);
}

std::vector<uintptr_t> CameraSystem::ScanMemoryPattern(const std::vector<uint8_t>& pattern, 
                                                      const std::vector<bool>& mask) {
    std::vector<uintptr_t> results;
    
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    
    while (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect == PAGE_EXECUTE_READ || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
            
            SIZE_T bytesRead;
            std::vector<uint8_t> buffer(mbi.RegionSize);
            
            if (ReadProcessMemory(processHandle, mbi.BaseAddress, buffer.data(), 
                                mbi.RegionSize, &bytesRead)) {
                
                for (size_t i = 0; i < bytesRead - pattern.size(); ++i) {
                    bool match = true;
                    
                    for (size_t j = 0; j < pattern.size(); ++j) {
                        if (mask[j] && buffer[i + j] != pattern[j]) {
                            match = false;
                            break;
                        }
                    }
                    
                    if (match) {
                        results.push_back((uintptr_t)mbi.BaseAddress + i);
                    }
                }
            }
        }
        
        address += mbi.RegionSize;
        if (address == 0) break; // Overflow protection
    }
    
    return results;
}

bool CameraSystem::GetCameraState(CameraState& state) {
    if (!processHandle || cameraBaseAddress == 0) return false;
    
    // Read position (typically at offset 0x0)
    if (!ReadMemory(cameraBaseAddress, &state.position, sizeof(XMFLOAT3))) {
        return false;
    }
    
    // Read rotation (typically at offset 0xC)  
    if (!ReadMemory(cameraBaseAddress + 0xC, &state.rotation, sizeof(XMFLOAT3))) {
        return false;
    }
    
    // Read FOV if available
    if (fovAddress != 0) {
        ReadValue(fovAddress, state.fov);
    }
    
    return true;
}

bool CameraSystem::SetCameraState(const CameraState& state) {
    if (!processHandle || cameraBaseAddress == 0) return false;
    
    if (safetyMode && !ValidateCameraState(state)) {
        std::cout << "Camera state validation failed" << std::endl;
        return false;
    }
    
    // Write position
    if (!WriteMemory(cameraBaseAddress, &state.position, sizeof(XMFLOAT3))) {
        return false;
    }
    
    // Write rotation
    if (!WriteMemory(cameraBaseAddress + 0xC, &state.rotation, sizeof(XMFLOAT3))) {
        return false;
    }
    
    // Write FOV if available
    if (fovAddress != 0) {
        WriteValue(fovAddress, state.fov);
    }
    
    currentState = state;
    return true;
}

bool CameraSystem::SetCameraPosition(const XMFLOAT3& position) {
    CameraState state = currentState;
    state.position = position;
    return SetCameraState(state);
}

bool CameraSystem::SetCameraRotation(const XMFLOAT3& rotation) {
    CameraState state = currentState;
    state.rotation = rotation;
    return SetCameraState(state);
}

bool CameraSystem::GetFOV(float& fov) {
    if (fovAddress == 0) return false;
    return ReadValue(fovAddress, fov);
}

bool CameraSystem::SetFOV(float fov) {
    if (fovAddress == 0) return false;
    
    // Clamp FOV to safe range
    fov = std::clamp(fov, minFOV, maxFOV);
    
    if (WriteValue(fovAddress, fov)) {
        currentState.fov = fov;
        return true;
    }
    return false;
}

bool CameraSystem::AdjustFOV(float delta) {
    float currentFOV;
    if (!GetFOV(currentFOV)) return false;
    
    return SetFOV(currentFOV + delta);
}

void CameraSystem::SetFOVLimits(float minFov, float maxFov) {
    minFOV = std::max(10.0f, minFov);
    maxFOV = std::min(179.0f, maxFov);
}

bool CameraSystem::EnableFreeCamera(bool enable) {
    if (enable && !freeCameraEnabled) {
        // Backup original camera state
        if (!GetCameraState(originalState)) {
            std::cout << "Failed to backup camera state" << std::endl;
            return false;
        }
        
        freeCameraEnabled = true;
        originalCameraBackup = true;
        std::cout << "Free camera enabled" << std::endl;
        
    } else if (!enable && freeCameraEnabled) {
        freeCameraEnabled = false;
        
        // Restore original camera state
        if (originalCameraBackup) {
            SetCameraState(originalState);
            std::cout << "Original camera restored" << std::endl;
        }
    }
    
    return true;
}

void CameraSystem::UpdateFreeCamera() {
    if (!freeCameraEnabled) return;
    
    UpdateInput();
    ProcessKeyboard();
    ProcessMouse();
    
    // Apply velocity to position
    XMFLOAT3 newPosition = currentState.position;
    newPosition.x += freeCameraVelocity.x * deltaTime;
    newPosition.y += freeCameraVelocity.y * deltaTime;
    newPosition.z += freeCameraVelocity.z * deltaTime;
    
    // Apply damping to velocity
    const float damping = 0.9f;
    freeCameraVelocity.x *= damping;
    freeCameraVelocity.y *= damping;
    freeCameraVelocity.z *= damping;
    
    SetCameraPosition(newPosition);
}

void CameraSystem::SetFreeCameraSpeed(float speed) {
    currentState.speed = std::clamp(speed, 0.1f, maxSpeed);
}

void CameraSystem::UpdateInput() {
    // Update keyboard state
    for (int i = 0; i < 256; ++i) {
        input.keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
    
    // Update mouse state
    POINT mousePos;
    GetCursorPos(&mousePos);
    
    input.mouseDeltaX = mousePos.x - input.mouseX;
    input.mouseDeltaY = mousePos.y - input.mouseY;
    input.mouseX = mousePos.x;
    input.mouseY = mousePos.y;
    
    // Update mouse buttons
    input.mouseButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    input.mouseButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    input.mouseButtons[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
}

void CameraSystem::ProcessKeyboard() {
    if (!freeCameraEnabled) return;
    
    const float speed = currentState.speed;
    const float acceleration = speed * 10.0f;
    
    // Calculate movement direction based on camera rotation
    XMFLOAT3 forward = EulerToDirection(currentState.rotation);
    XMFLOAT3 right = CameraUtils::CrossProduct(forward, {0.0f, 1.0f, 0.0f});
    XMFLOAT3 up = {0.0f, 1.0f, 0.0f};
    
    // Movement input
    if (input.keys['W']) {
        freeCameraVelocity.x += forward.x * acceleration * deltaTime;
        freeCameraVelocity.y += forward.y * acceleration * deltaTime;
        freeCameraVelocity.z += forward.z * acceleration * deltaTime;
    }
    if (input.keys['S']) {
        freeCameraVelocity.x -= forward.x * acceleration * deltaTime;
        freeCameraVelocity.y -= forward.y * acceleration * deltaTime;
        freeCameraVelocity.z -= forward.z * acceleration * deltaTime;
    }
    if (input.keys['A']) {
        freeCameraVelocity.x -= right.x * acceleration * deltaTime;
        freeCameraVelocity.y -= right.y * acceleration * deltaTime;
        freeCameraVelocity.z -= right.z * acceleration * deltaTime;
    }
    if (input.keys['D']) {
        freeCameraVelocity.x += right.x * acceleration * deltaTime;
        freeCameraVelocity.y += right.y * acceleration * deltaTime;
        freeCameraVelocity.z += right.z * acceleration * deltaTime;
    }
    if (input.keys['Q']) {
        freeCameraVelocity.y -= acceleration * deltaTime;
    }
    if (input.keys['E']) {
        freeCameraVelocity.y += acceleration * deltaTime;
    }
    
    // Speed adjustment
    if (input.keys[VK_SHIFT]) {
        SetFreeCameraSpeed(currentState.speed * 2.0f);
    }
    if (input.keys[VK_CONTROL]) {
        SetFreeCameraSpeed(currentState.speed * 0.5f);
    }
}

void CameraSystem::ProcessMouse() {
    if (!freeCameraEnabled || !input.mouseButtons[1]) return; // Right mouse button for look
    
    const float sensitivity = currentState.sensitivity * 0.001f;
    
    XMFLOAT3 rotation = currentState.rotation;
    rotation.y += input.mouseDeltaX * sensitivity; // Yaw
    rotation.x += input.mouseDeltaY * sensitivity * (currentState.invertY ? 1.0f : -1.0f); // Pitch
    
    // Clamp pitch to prevent gimbal lock
    rotation.x = std::clamp(rotation.x, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);
    
    SetCameraRotation(rotation);
}

void CameraSystem::Update() {
    auto now = std::chrono::steady_clock::now();
    deltaTime = std::chrono::duration<float>(now - lastUpdate).count();
    lastUpdate = now;
    
    // Update free camera
    if (freeCameraEnabled) {
        UpdateFreeCamera();
    }
    
    // Update transitions
    UpdateTransitions();
}

void CameraSystem::StartCameraTransition(const CameraState& targetState, float duration, 
                                        CameraTransition::EaseType easeType) {
    GetCameraState(activeTransition.startState);
    activeTransition.endState = targetState;
    activeTransition.duration = duration;
    activeTransition.currentTime = 0.0f;
    activeTransition.easeType = easeType;
    activeTransition.isActive = true;
    
    std::cout << "Started camera transition (duration: " << duration << "s)" << std::endl;
}

void CameraSystem::UpdateTransitions() {
    if (!activeTransition.isActive) return;
    
    activeTransition.currentTime += deltaTime;
    float t = activeTransition.currentTime / activeTransition.duration;
    
    if (t >= 1.0f) {
        // Transition complete
        SetCameraState(activeTransition.endState);
        activeTransition.isActive = false;
        std::cout << "Camera transition completed" << std::endl;
        return;
    }
    
    // Apply easing function
    float easedT = EaseFunction(t, activeTransition.easeType);
    
    // Interpolate camera state
    CameraState interpolatedState;
    interpolatedState.position = LerpFloat3(activeTransition.startState.position, 
                                           activeTransition.endState.position, easedT);
    interpolatedState.rotation = LerpFloat3(activeTransition.startState.rotation, 
                                           activeTransition.endState.rotation, easedT);
    interpolatedState.fov = activeTransition.startState.fov + 
                           (activeTransition.endState.fov - activeTransition.startState.fov) * easedT;
    
    SetCameraState(interpolatedState);
}

void CameraSystem::StopTransition() {
    activeTransition.isActive = false;
    std::cout << "Camera transition stopped" << std::endl;
}

void CameraSystem::RestoreOriginalCamera() {
    if (originalCameraBackup) {
        SetCameraState(originalState);
        std::cout << "Original camera state restored" << std::endl;
    }
}

// Utility template implementations
template<typename T>
bool CameraSystem::ReadValue(uintptr_t address, T& value) {
    SIZE_T bytesRead;
    return ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(T), &bytesRead) 
           && bytesRead == sizeof(T);
}

template<typename T>
bool CameraSystem::WriteValue(uintptr_t address, const T& value) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(T), &bytesWritten) 
           && bytesWritten == sizeof(T);
}

bool CameraSystem::ReadMemory(uintptr_t address, void* buffer, size_t size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(processHandle, (LPCVOID)address, buffer, size, &bytesRead) 
           && bytesRead == size;
}

bool CameraSystem::WriteMemory(uintptr_t address, const void* buffer, size_t size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, (LPVOID)address, buffer, size, &bytesWritten) 
           && bytesWritten == size;
}

// Math utility implementations
XMFLOAT3 CameraSystem::EulerToDirection(const XMFLOAT3& euler) {
    float pitch = euler.x;
    float yaw = euler.y;
    
    XMFLOAT3 direction;
    direction.x = cos(pitch) * cos(yaw);
    direction.y = sin(pitch);
    direction.z = cos(pitch) * sin(yaw);
    
    return direction;
}

XMFLOAT3 CameraSystem::DirectionToEuler(const XMFLOAT3& direction) {
    XMFLOAT3 euler;
    euler.x = asin(direction.y); // Pitch
    euler.y = atan2(direction.z, direction.x); // Yaw
    euler.z = 0.0f; // Roll
    
    return euler;
}

float CameraSystem::EaseFunction(float t, CameraTransition::EaseType type) {
    switch (type) {
        case CameraTransition::EaseType::Linear:
            return t;
            
        case CameraTransition::EaseType::EaseIn:
            return t * t;
            
        case CameraTransition::EaseType::EaseOut:
            return 1.0f - (1.0f - t) * (1.0f - t);
            
        case CameraTransition::EaseType::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
            
        case CameraTransition::EaseType::Bounce:
            if (t < 0.5f) {
                return 0.5f * (1.0f - cos(t * XM_PI * 4.0f));
            } else {
                return 0.5f + 0.5f * (1.0f - cos((t - 0.5f) * XM_PI * 4.0f));
            }
            
        case CameraTransition::EaseType::Elastic:
            if (t == 0.0f || t == 1.0f) return t;
            return pow(2.0f, -10.0f * t) * sin((t - 0.1f) * 2.0f * XM_PI / 0.4f) + 1.0f;
            
        default:
            return t;
    }
}

XMFLOAT3 CameraSystem::LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

bool CameraSystem::ValidateCameraState(const CameraState& state) {
    // Check for NaN or infinite values
    if (!isfinite(state.position.x) || !isfinite(state.position.y) || !isfinite(state.position.z)) {
        return false;
    }
    if (!isfinite(state.rotation.x) || !isfinite(state.rotation.y) || !isfinite(state.rotation.z)) {
        return false;
    }
    if (!isfinite(state.fov) || state.fov <= 0.0f) {
        return false;
    }
    
    // Check reasonable ranges
    if (abs(state.position.x) > 1000000.0f || abs(state.position.y) > 1000000.0f || abs(state.position.z) > 1000000.0f) {
        return false;
    }
    
    return true;
}

// CameraUtils namespace implementations
namespace CameraUtils {
    float RadiansToDegrees(float radians) {
        return radians * 180.0f / XM_PI;
    }
    
    float DegreesToRadians(float degrees) {
        return degrees * XM_PI / 180.0f;
    }
    
    XMFLOAT3 CrossProduct(const XMFLOAT3& a, const XMFLOAT3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
    
    float DotProduct(const XMFLOAT3& a, const XMFLOAT3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    XMFLOAT3 Normalize(const XMFLOAT3& v) {
        float length = Length(v);
        if (length > 0.0f) {
            return {v.x / length, v.y / length, v.z / length};
        }
        return {0.0f, 0.0f, 0.0f};
    }
    
    float Length(const XMFLOAT3& v) {
        return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    
    XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float t) {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
    }
    
    bool IsValidFOV(float fov) {
        return fov > 10.0f && fov < 180.0f && isfinite(fov);
    }
    
    bool IsValidPosition(const XMFLOAT3& position) {
        return isfinite(position.x) && isfinite(position.y) && isfinite(position.z) &&
               abs(position.x) < 1000000.0f && abs(position.y) < 1000000.0f && abs(position.z) < 1000000.0f;
    }
    
    bool IsValidRotation(const XMFLOAT3& rotation) {
        return isfinite(rotation.x) && isfinite(rotation.y) && isfinite(rotation.z);
    }
}