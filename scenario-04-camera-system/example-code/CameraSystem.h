#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <functional>

using namespace DirectX;

/**
 * Advanced Camera System for Game Modification
 * 
 * This system provides comprehensive camera control including:
 * - Free camera movement with smooth interpolation
 * - FOV adjustment with game-specific safety checks
 * - Camera tracking and follow modes
 * - Cinematic camera sequences
 * - Photo mode with enhanced controls
 */

struct CameraState {
    XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3 rotation = {0.0f, 0.0f, 0.0f};  // Euler angles (pitch, yaw, roll)
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    // Additional camera properties
    float speed = 1.0f;
    float sensitivity = 1.0f;
    bool invertY = false;
};

struct CameraTransition {
    CameraState startState;
    CameraState endState;
    float duration;
    float currentTime;
    
    enum class EaseType {
        Linear,
        EaseIn,
        EaseOut,
        EaseInOut,
        Bounce,
        Elastic
    };
    
    EaseType easeType = EaseType::EaseInOut;
    bool isActive = false;
};

class CameraSystem {
private:
    // Process and memory management
    HANDLE processHandle;
    std::wstring processName;
    uintptr_t cameraBaseAddress;
    uintptr_t fovAddress;
    
    // Camera state management
    CameraState currentState;
    CameraState originalState;
    CameraTransition activeTransition;
    
    // Free camera mode
    bool freeCameraEnabled;
    bool originalCameraBackup;
    XMFLOAT3 freeCameraVelocity;
    
    // Input handling
    struct InputState {
        bool keys[256];
        int mouseX, mouseY;
        int mouseDeltaX, mouseDeltaY;
        bool mouseButtons[3];
        float mouseWheel;
    };
    InputState input;
    
    // Performance and safety
    std::chrono::steady_clock::time_point lastUpdate;
    float deltaTime;
    bool safetyMode;
    float maxSpeed;
    float maxFOV;
    float minFOV;

public:
    CameraSystem();
    ~CameraSystem();
    
    // Initialization and cleanup
    bool Initialize(const std::wstring& targetProcess);
    void Shutdown();
    bool IsInitialized() const { return processHandle != nullptr; }
    
    // Camera position and rotation
    bool GetCameraState(CameraState& state);
    bool SetCameraState(const CameraState& state);
    bool SetCameraPosition(const XMFLOAT3& position);
    bool SetCameraRotation(const XMFLOAT3& rotation);
    
    // FOV control
    bool GetFOV(float& fov);
    bool SetFOV(float fov);
    bool AdjustFOV(float delta);
    void SetFOVLimits(float minFOV, float maxFOV);
    
    // Free camera mode
    bool EnableFreeCamera(bool enable);
    bool IsFreeCameraEnabled() const { return freeCameraEnabled; }
    void UpdateFreeCamera();
    void SetFreeCameraSpeed(float speed);
    
    // Camera transitions and animation
    void StartCameraTransition(const CameraState& targetState, float duration, 
                              CameraTransition::EaseType easeType = CameraTransition::EaseType::EaseInOut);
    void UpdateTransitions();
    bool IsTransitionActive() const { return activeTransition.isActive; }
    void StopTransition();
    
    // Input handling
    void UpdateInput();
    void ProcessKeyboard();
    void ProcessMouse();
    
    // Utility functions
    void Update();
    void RestoreOriginalCamera();
    void SetSafetyMode(bool enabled) { safetyMode = enabled; }
    bool GetSafetyMode() const { return safetyMode; }
    
    // Camera presets and positions
    void SaveCameraPreset(const std::string& name);
    bool LoadCameraPreset(const std::string& name);
    std::vector<std::string> GetAvailablePresets();
    
private:
    // Memory operations
    bool ReadMemory(uintptr_t address, void* buffer, size_t size);
    bool WriteMemory(uintptr_t address, const void* buffer, size_t size);
    template<typename T>
    bool ReadValue(uintptr_t address, T& value);
    template<typename T>
    bool WriteValue(uintptr_t address, const T& value);
    
    // Address scanning
    bool FindCameraBaseAddress();
    bool FindFOVAddress();
    std::vector<uintptr_t> ScanMemoryPattern(const std::vector<uint8_t>& pattern, 
                                            const std::vector<bool>& mask);
    
    // Math utilities
    XMFLOAT3 EulerToDirection(const XMFLOAT3& euler);
    XMFLOAT3 DirectionToEuler(const XMFLOAT3& direction);
    float EaseFunction(float t, CameraTransition::EaseType type);
    XMFLOAT3 LerpFloat3(const XMFLOAT3& a, const XMFLOAT3& b, float t);
    
    // Validation and safety
    bool ValidateCameraState(const CameraState& state);
    bool IsPositionSafe(const XMFLOAT3& position);
    void ClampCameraState(CameraState& state);
};

/**
 * Advanced Camera Controller with cinematic features
 */
class CinematicCamera {
private:
    struct Waypoint {
        CameraState state;
        float duration;
        CameraTransition::EaseType easeType;
    };
    
    std::vector<Waypoint> waypoints;
    int currentWaypoint;
    bool isPlaying;
    bool looping;
    float playbackSpeed;
    
    CameraSystem* cameraSystem;

public:
    CinematicCamera(CameraSystem* system);
    
    // Waypoint management
    void AddWaypoint(const CameraState& state, float duration, 
                    CameraTransition::EaseType easeType = CameraTransition::EaseType::EaseInOut);
    void RemoveWaypoint(int index);
    void ClearWaypoints();
    int GetWaypointCount() const { return static_cast<int>(waypoints.size()); }
    
    // Playback control
    void Play();
    void Pause();
    void Stop();
    void SetPlaybackSpeed(float speed) { playbackSpeed = speed; }
    void SetLooping(bool enable) { looping = enable; }
    
    // Update
    void Update();
    
    // Status
    bool IsPlaying() const { return isPlaying; }
    int GetCurrentWaypoint() const { return currentWaypoint; }
    float GetProgress() const;
};

/**
 * Photo Mode - Enhanced camera controls for screenshot capture
 */
class PhotoMode {
private:
    CameraSystem* cameraSystem;
    CameraState photoModeState;
    CameraState gameState;
    bool isActive;
    bool hideUI;
    bool pauseGame;
    
    // Photo mode specific settings
    float depthOfField;
    float exposure;
    float contrast;
    float saturation;
    bool orthographicMode;
    float orthographicSize;

public:
    PhotoMode(CameraSystem* system);
    
    // Photo mode control
    bool EnterPhotoMode();
    bool ExitPhotoMode();
    bool IsActive() const { return isActive; }
    
    // Camera controls
    void SetDepthOfField(float dof) { depthOfField = dof; }
    void SetExposure(float exp) { exposure = exp; }
    void SetContrast(float cont) { contrast = cont; }
    void SetSaturation(float sat) { saturation = sat; }
    
    // View modes
    void SetOrthographicMode(bool enable);
    void SetOrthographicSize(float size) { orthographicSize = size; }
    
    // Utility
    void TakeScreenshot(const std::string& filename);
    void SetUIVisibility(bool visible) { hideUI = !visible; }
    void SetGamePause(bool pause) { pauseGame = pause; }
    
    void Update();
};

/**
 * Camera Tracking System - Follow objects or characters
 */
class CameraTracker {
public:
    enum class TrackingMode {
        None,
        Position,      // Follow position only
        LookAt,        // Look at target
        Follow,        // Follow with offset
        Orbit,         // Orbit around target
        FirstPerson,   // First person view
        ThirdPerson    // Third person view
    };
    
    struct TrackingSettings {
        TrackingMode mode = TrackingMode::None;
        XMFLOAT3 offset = {0.0f, 0.0f, 0.0f};
        float distance = 5.0f;
        float height = 2.0f;
        float smoothing = 1.0f;
        bool collision = true;
        float orbitSpeed = 1.0f;
    };

private:
    CameraSystem* cameraSystem;
    uintptr_t targetAddress;
    TrackingSettings settings;
    XMFLOAT3 targetPosition;
    XMFLOAT3 smoothedPosition;
    XMFLOAT3 orbitAngles;

public:
    CameraTracker(CameraSystem* system);
    
    // Target management
    bool SetTarget(uintptr_t address);
    bool SetTargetByName(const std::string& objectName);
    void ClearTarget();
    bool HasTarget() const { return targetAddress != 0; }
    
    // Tracking configuration
    void SetTrackingMode(TrackingMode mode);
    void SetTrackingSettings(const TrackingSettings& settings);
    TrackingSettings& GetTrackingSettings() { return settings; }
    
    // Update
    void Update();
    
private:
    bool GetTargetPosition(XMFLOAT3& position);
    XMFLOAT3 CalculateCameraPosition();
    XMFLOAT3 CalculateLookDirection();
    bool CheckCollision(const XMFLOAT3& from, const XMFLOAT3& to);
};

/**
 * Utility classes for camera system
 */
namespace CameraUtils {
    // Game-specific camera patterns for different engines
    namespace Patterns {
        // Unreal Engine camera patterns
        extern const std::vector<uint8_t> UE4_CAMERA_PATTERN;
        extern const std::vector<bool> UE4_CAMERA_MASK;
        
        // Unity engine camera patterns  
        extern const std::vector<uint8_t> UNITY_CAMERA_PATTERN;
        extern const std::vector<bool> UNITY_CAMERA_MASK;
        
        // Custom game patterns (add as needed)
        extern const std::vector<uint8_t> ELDENRING_CAMERA_PATTERN;
        extern const std::vector<bool> ELDENRING_CAMERA_MASK;
    }
    
    // Math utilities
    float RadiansToDegrees(float radians);
    float DegreesToRadians(float degrees);
    XMFLOAT3 CrossProduct(const XMFLOAT3& a, const XMFLOAT3& b);
    float DotProduct(const XMFLOAT3& a, const XMFLOAT3& b);
    XMFLOAT3 Normalize(const XMFLOAT3& v);
    float Length(const XMFLOAT3& v);
    XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float t);
    
    // Camera validation
    bool IsValidFOV(float fov);
    bool IsValidPosition(const XMFLOAT3& position);
    bool IsValidRotation(const XMFLOAT3& rotation);
    
    // Conversion utilities
    XMMATRIX StateToMatrix(const CameraState& state);
    CameraState MatrixToState(const XMMATRIX& matrix);
}