/*
 * Exercise 5: 포토 모드
 * 
 * 문제: 게임을 일시정지하고 카메라를 자유롭게 조작할 수 있는 포토 모드를 만드세요.
 * 
 * 학습 목표:
 * - 게임 일시정지 시스템
 * - 통합 카메라 제어
 * - 포토그래피 도구
 */

#include <Windows.h>
#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi.h>
#include <detours.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>
#include <cmath> // For std::abs, std::min, std::max, std::cos, std::sin, std::sqrt, std::atan2
#include <limits> // For std::numeric_limits
#include <codecvt> // For std::wstring_convert
#include <locale> // For std::wstring_convert

#pragma comment(lib, "detours.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

struct CameraData {
    XMFLOAT3 position;      // 0x00: 카메라 위치
    XMFLOAT3 rotation;      // 0x0C: 오일러 각도
    float fov;              // 0x18: 시야각 (radians)
    float nearPlane;        // 0x1C: 근거리 클리핑
    float farPlane;         // 0x20: 원거리 클리핑
    float aspectRatio;      // 0x24: 화면 비율
    char padding[8];        // 0x28: 패딩
};

struct GameTimeData {
    float timeScale;        // 시간 스케일 (0.0 = 일시정지)
    float deltaTime;        // 프레임 시간
    float totalTime;        // 총 경과 시간
    bool isPaused;          // 일시정지 상태
};

enum class PhotoMode {
    Disabled,
    FreeCam,
    OrbitCam,
    FixedCam,
    CinematicCam
};

struct PhotoSettings {
    // 카메라 설정
    float movementSpeed = 5.0f;
    float rotationSpeed = 90.0f;
    float mouseSensitivity = 0.1f;
    float zoomSpeed = 2.0f;
    
    // 렌더링 설정
    float brightness = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    float vignette = 0.0f;
    float bloom = 0.0f;
    float depthOfField = 0.0f;
    
    // 필터 설정
    bool enableBlackWhite = false;
    bool enableSepia = false;
    bool enableVintage = false;
    XMFLOAT3 colorTint = {1.0f, 1.0f, 1.0f};
    
    // UI 설정
    bool hideUI = true;
    bool hidePlayer = false;
    bool hideEnemies = false;
    bool showGrid = false;
    bool showRuleOfThirds = false;
};

struct PhotoBookmark {
    std::string name;
    CameraData camera;
    PhotoSettings settings;
    std::string timestamp;
    std::string description;
};

class PhotoModeSystem {
private:
    static PhotoModeSystem* instance;
    
    // 시스템 상태
    bool isInitialized;
    PhotoMode currentMode;
    bool isActive;
    
    // 카메라 데이터
    uintptr_t cameraAddress;
    CameraData originalCamera;
    CameraData currentCamera;
    
    // 게임 시간 제어
    uintptr_t timeAddress;
    GameTimeData originalTime;
    GameTimeData currentTime;
    
    // 포토 설정
    PhotoSettings settings;
    std::vector<PhotoBookmark> bookmarks;
    int selectedBookmark;
    
    // 입력 시스템
    std::map<int, bool> keyStates;
    std::map<int, bool> previousKeyStates;
    POINT lastMousePos;
    int mouseWheelDelta;
    bool isFirstMouseInput;
    
    // UI 시스템
    bool showUI;
    bool showSettings;
    bool showHelp;
    int selectedSetting;
    std::chrono::steady_clock::time_point lastUIUpdate;
    
    // 스레드 관리
    std::thread inputThread;
    std::thread uiThread;
    std::atomic<bool> threadsRunning;
    
    // 스크린샷 시스템
    struct ScreenshotData {
        std::string filename;
        int width;
        int height;
        int quality;
        bool includeMetadata;
    };
    
    std::vector<ScreenshotData> screenshotQueue;
    
    // 성능 모니터링
    struct PerformanceStats {
        float fps;
        float frameTime;
        int screenshotCount;
        std::chrono::steady_clock::time_point sessionStart;
    };
    
    PerformanceStats stats;
    
    // 오빗 카메라 (타겟 중심 회전)
    struct OrbitCameraData {
        XMFLOAT3 target;
        float distance;
        float pitch;
        float yaw;
        bool isActive;
    };
    
    OrbitCameraData orbitCamera;

public:
    PhotoModeSystem() :
        isInitialized(false),
        currentMode(PhotoMode::Disabled),
        isActive(false),
        cameraAddress(0),
        timeAddress(0),
        selectedBookmark(0),
        isFirstMouseInput(true),
        showUI(true),
        showSettings(false),
        showHelp(false),
        selectedSetting(0),
        mouseWheelDelta(0),
        threadsRunning(false) {
        
        instance = this;
        
        // 기본 설정 초기화
        settings = PhotoSettings();
        
        // 오빗 카메라 초기화
        orbitCamera.isActive = false;
        orbitCamera.distance = 10.0f;
        orbitCamera.pitch = 0.0f;
        orbitCamera.yaw = 0.0f;
        
        // 성능 통계 초기화
        stats.fps = 0.0f;
        stats.frameTime = 0.0f;
        stats.screenshotCount = 0;
        stats.sessionStart = std::chrono::steady_clock::now();
        
        GetCursorPos(&lastMousePos);
    }
    
    ~PhotoModeSystem() {
        Shutdown();
        instance = nullptr;
    }
    
    static PhotoModeSystem* GetInstance() {
        return instance;
    }
    
    bool Initialize() {
        std::wcout << L"포토 모드 시스템 초기화 중..." << std::endl;
        
        // 카메라 주소 찾기
        if (!FindCameraAddress()) {
            std::wcout << L"카메라 주소를 찾을 수 없습니다." << std::endl;
            return false;
        }
        
        // 시간 시스템 주소 찾기
        if (!FindTimeAddress()) {
            std::wcout << L"시간 시스템 주소를 찾을 수 없습니다." << std::endl;
            return false;
        }
        
        // 원본 상태 백업
        // ReadProcessMemory(GetCurrentProcess(), ... ) 대신 실제 게임 프로세스 핸들 사용 필요
        // 여기서는 예시로 GetCurrentProcess() 사용
        if (!ReadCameraData() || !ReadTimeData()) {
            std::wcout << L"게임 데이터를 읽을 수 없습니다." << std::endl;
            return false;
        }
        
        originalCamera = currentCamera;
        originalTime = currentTime;
        
        // 설정 로드
        LoadSettings();
        LoadBookmarks();
        
        // 스레드 시작
        threadsRunning = true;
        inputThread = std::thread(&PhotoModeSystem::InputThreadFunction, this);
        uiThread = std::thread(&PhotoModeSystem::UIThreadFunction, this);
        
        isInitialized = true;
        std::wcout << L"포토 모드 시스템 초기화 완료" << std::endl;
        PrintControls();
        
        return true;
    }
    
    void Shutdown() {
        if (!isInitialized) return;
        
        // 포토 모드 비활성화
        if (isActive) {
            DeactivatePhotoMode();
        }
        
        // 설정 저장
        SaveSettings();
        SaveBookmarks();
        
        // 스레드 종료
        threadsRunning = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
        if (uiThread.joinable()) {
            uiThread.join();
        }
        
        isInitialized = false;
        std::wcout << L"포토 모드 시스템 종료" << std::endl;
    }
    
    void Update() {
        if (!isInitialized) return;
        
        UpdatePerformanceStats();
        
        if (isActive) {
            UpdatePhotoMode();
            ProcessScreenshotQueue();
        }
    }
    
    void ActivatePhotoMode(PhotoMode mode = PhotoMode::FreeCam) {
        if (!isInitialized || isActive) return;
        
        currentMode = mode;
        isActive = true;
        
        // 게임 일시정지
        PauseGame(true);
        
        // 카메라 모드 설정
        SetupCameraMode(mode);
        
        // UI 초기화
        showUI = true;
        isFirstMouseInput = true;
        
        std::wcout << L"포토 모드 활성화: " << GetModeString(mode) << std::endl;
    }
    
    void DeactivatePhotoMode() {
        if (!isActive) return;
        
        // 원본 상태 복원
        RestoreOriginalState();
        
        // 게임 재개
        PauseGame(false);
        
        isActive = false;
        currentMode = PhotoMode::Disabled;
        
        std::wcout << L"포토 모드 비활성화" << std::endl;
    }
    
    void TogglePhotoMode() {
        if (isActive) {
            DeactivatePhotoMode();
        } else {
            ActivatePhotoMode();
        }
    }
    
    void SwitchMode(PhotoMode newMode) {
        if (!isActive) return;
        
        currentMode = newMode;
        SetupCameraMode(newMode);
        
        std::wcout << L"카메라 모드 변경: " << GetModeString(newMode) << std::endl;
    }
    
    void TakeScreenshot(const std::string& filename = "", bool includeMetadata = true) {
        if (!isActive) return;
        
        ScreenshotData screenshot;
        
        if (filename.empty()) {
            // 자동 파일명 생성 (timestamp 기반)
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "photo_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".png";
            screenshot.filename = ss.str();
        } else {
            screenshot.filename = filename;
        }
        
        screenshot.width = 1920;  // 기본 해상도
        screenshot.height = 1080;
        screenshot.quality = 95;
        screenshot.includeMetadata = includeMetadata;
        
        screenshotQueue.push_back(screenshot);
        
        std::wcout << L"스크린샷 촬영: " << StringToWString(screenshot.filename) << std::endl;
        stats.screenshotCount++;
    }
    
    // 북마크 시스템
    void SaveBookmark(const std::string& name, const std::string& description = "") {
        PhotoBookmark bookmark;
        bookmark.name = name;
        bookmark.camera = currentCamera;
        bookmark.settings = settings;
        bookmark.description = description;
        
        // 타임스탬프 생성
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        bookmark.timestamp = ss.str();
        
        bookmarks.push_back(bookmark);
        
        std::wcout << L"북마크 저장: " << StringToWString(name) << std::endl;
    }
    
    void LoadBookmark(int index) {
        if (index >= 0 && index < bookmarks.size()) {
            const auto& bookmark = bookmarks[index];
            
            currentCamera = bookmark.camera;
            settings = bookmark.settings;
            
            // WriteProcessMemory(GetCurrentProcess(), ... ) 대신 실제 게임 프로세스 핸들 사용 필요
            WriteCameraData();
            
            std::wcout << L"북마크 로드: " << StringToWString(bookmark.name) << std::endl;
        }
    }
    
    void DeleteBookmark(int index) {
        if (index >= 0 && index < bookmarks.size()) {
            std::wcout << L"북마크 삭제: " << StringToWString(bookmarks[index].name) << std::endl;
            bookmarks.erase(bookmarks.begin() + index);
        }
    }

private:
    bool FindCameraAddress() {
        cameraAddress = 0x7FF700000000;  // 예제 주소
        
        std::ifstream configFile("camera_address.txt");
        if (configFile.is_open()) {
            configFile >> std::hex >> cameraAddress;
            configFile.close();
        }
        
        return cameraAddress != 0;
    }
    
    bool FindTimeAddress() {
        timeAddress = 0x7FF700001000;  // 예제 주소
        
        std::ifstream configFile("time_address.txt");
        if (configFile.is_open()) {
            configFile >> std::hex >> timeAddress;
            configFile.close();
        }
        
        return timeAddress != 0;
    }
    
    bool ReadCameraData() {
        if (cameraAddress == 0) return false;
        
        SIZE_T bytesRead;
        return ReadProcessMemory(GetCurrentProcess(), 
                               reinterpret_cast<LPCVOID>(cameraAddress),
                               &currentCamera, sizeof(CameraData), &bytesRead);
    }
    
    bool WriteCameraData() {
        if (cameraAddress == 0) return false;
        
        SIZE_T bytesWritten;
        return WriteProcessMemory(GetCurrentProcess(),
                                reinterpret_cast<LPVOID>(cameraAddress),
                                &currentCamera, sizeof(CameraData), &bytesWritten);
    }
    
    bool ReadTimeData() {
        if (timeAddress == 0) return false;
        
        SIZE_T bytesRead;
        return ReadProcessMemory(GetCurrentProcess(),
                               reinterpret_cast<LPCVOID>(timeAddress),
                               &currentTime, sizeof(GameTimeData), &bytesRead);
    }
    
    bool WriteTimeData() {
        if (timeAddress == 0) return false;
        
        SIZE_T bytesWritten;
        return WriteProcessMemory(GetCurrentProcess(),
                                reinterpret_cast<LPVOID>(timeAddress),
                                &currentTime, sizeof(GameTimeData), &bytesWritten);
    }
    
    void RestoreOriginalState() {
        currentCamera = originalCamera;
        currentTime = originalTime;
        
        WriteCameraData();
        WriteTimeData();
    }
    
    void PauseGame(bool pause) {
        currentTime.isPaused = pause;
        currentTime.timeScale = pause ? 0.0f : 1.0f;
        WriteTimeData();
        
        std::wcout << (pause ? L"게임 일시정지" : L"게임 재개") << std::endl;
    }
    
    void SetupCameraMode(PhotoMode mode) {
        switch (mode) {
            case PhotoMode::FreeCam:
                // 자유 카메라 설정
                break;
                
            case PhotoMode::OrbitCam:
                // 오빗 카메라 설정
                orbitCamera.target = currentCamera.position;
                orbitCamera.isActive = true;
                break;
                
            case PhotoMode::FixedCam:
                // 고정 카메라 설정
                break;
                
            case PhotoMode::CinematicCam:
                // 시네마틱 카메라 설정
                break;
        }
    }
    
    void UpdatePhotoMode() {
        ProcessInput();
        
        switch (currentMode) {
            case PhotoMode::FreeCam:
                UpdateFreeCameraMode();
                break;
                
            case PhotoMode::OrbitCam:
                UpdateOrbitCameraMode();
                break;
                
            case PhotoMode::FixedCam:
                UpdateFixedCameraMode();
                break;
                
            case PhotoMode::CinematicCam:
                UpdateCinematicCameraMode();
                break;
        }
        
        WriteCameraData();
    }
    
    void UpdateFreeCameraMode() {
        // WASD 이동
        XMFLOAT3 movement = {0, 0, 0};
        float speed = settings.movementSpeed * 0.016f;  // ~60 FPS 가정
        
        if (IsKeyHeld('W')) movement.z += speed;
        if (IsKeyHeld('S')) movement.z -= speed;
        if (IsKeyHeld('A')) movement.x -= speed;
        if (IsKeyHeld('D')) movement.x += speed;
        if (IsKeyHeld(VK_SPACE)) movement.y += speed;
        if (IsKeyHeld('C')) movement.y -= speed;
        
        // 속도 조절
        if (IsKeyHeld(VK_SHIFT)) {
            movement.x *= 3.0f;
            movement.y *= 3.0f;
            movement.z *= 3.0f;
        }
        if (IsKeyHeld(VK_CONTROL)) {
            movement.x *= 0.3f;
            movement.y *= 0.3f;
            movement.z *= 0.3f;
        }
        
        // 카메라 방향 기준으로 이동
        ApplyMovement(movement);
        
        // 마우스 룩
        if (IsKeyHeld(VK_RBUTTON)) {
            ProcessMouseLook();
        }
        
        // 마우스 휠 FOV 조정
        if (mouseWheelDelta != 0) {
            float fovDelta = mouseWheelDelta * settings.zoomSpeed * 0.1f;
            currentCamera.fov = std::max(XMConvertToRadians(10.0f), 
                                        std::min(XMConvertToRadians(179.0f), 
                                                currentCamera.fov + XMConvertToRadians(fovDelta)));
            mouseWheelDelta = 0;
        }
    }
    
    void UpdateOrbitCameraMode() {
        if (!orbitCamera.isActive) return;
        
        // 마우스 입력으로 회전
        if (IsKeyHeld(VK_LBUTTON)) {
            POINT currentMousePos;
            GetCursorPos(&currentMousePos);
            
            if (!isFirstMouseInput) {
                int deltaX = currentMousePos.x - lastMousePos.x;
                int deltaY = currentMousePos.y - lastMousePos.y;
                
                orbitCamera.yaw += deltaX * settings.mouseSensitivity * 0.1f;
                orbitCamera.pitch += deltaY * settings.mouseSensitivity * 0.1f;
                
                // Pitch 제한
                orbitCamera.pitch = std::max(-XM_PIDIV2, std::min(XM_PIDIV2, orbitCamera.pitch));
            }
            
            lastMousePos = currentMousePos;
            isFirstMouseInput = false;
        }
        
        // 마우스 휠로 거리 조정
        if (mouseWheelDelta != 0) {
            orbitCamera.distance += mouseWheelDelta * settings.zoomSpeed * 0.1f;
            orbitCamera.distance = std::max(1.0f, std::min(100.0f, orbitCamera.distance));
            mouseWheelDelta = 0;
        }
        
        // WASD로 타겟 이동
        XMFLOAT3 targetMovement = {0, 0, 0};
        float speed = settings.movementSpeed * 0.016f;
        
        if (IsKeyHeld('W')) targetMovement.z += speed;
        if (IsKeyHeld('S')) targetMovement.z -= speed;
        if (IsKeyHeld('A')) targetMovement.x -= speed;
        if (IsKeyHeld('D')) targetMovement.x += speed;
        if (IsKeyHeld(VK_SPACE)) targetMovement.y += speed;
        if (IsKeyHeld('C')) targetMovement.y -= speed;
        
        orbitCamera.target.x += targetMovement.x;
        orbitCamera.target.y += targetMovement.y;
        orbitCamera.target.z += targetMovement.z;
        
        // 카메라 위치 계산
        float x = orbitCamera.target.x + orbitCamera.distance * std::cos(orbitCamera.pitch) * std::sin(orbitCamera.yaw);
        float y = orbitCamera.target.y + orbitCamera.distance * std::sin(orbitCamera.pitch);
        float z = orbitCamera.target.z + orbitCamera.distance * std::cos(orbitCamera.pitch) * std::cos(orbitCamera.yaw);
        
        currentCamera.position = {x, y, z};
        
        // 타겟을 바라보도록 회전 설정
        XMFLOAT3 direction = {
            orbitCamera.target.x - x,
            orbitCamera.target.y - y,
            orbitCamera.target.z - z
        };
        
        currentCamera.rotation.y = std::atan2(direction.x, direction.z);
        currentCamera.rotation.x = -std::atan2(direction.y, std::sqrt(direction.x * direction.x + direction.z * direction.z));
    }
    
    void UpdateFixedCameraMode() {
        // 고정 카메라 모드 - 위치는 고정, 회전만 가능
        if (IsKeyHeld(VK_RBUTTON)) {
            ProcessMouseLook();
        }
        
        // FOV 조정만 허용
        if (mouseWheelDelta != 0) {
            float fovDelta = mouseWheelDelta * settings.zoomSpeed * 0.1f;
            currentCamera.fov = std::max(XMConvertToRadians(10.0f), 
                                        std::min(XMConvertToRadians(179.0f), 
                                                currentCamera.fov + XMConvertToRadians(fovDelta)));
            mouseWheelDelta = 0;
        }
    }
    
    void UpdateCinematicCameraMode() {
        // 시네마틱 카메라 - 자동 애니메이션이나 키프레임 기반
        // 여기서는 기본 구현만
        UpdateFreeCameraMode();
    }
    
    void ApplyMovement(const XMFLOAT3& movement) {
        if (movement.x == 0 && movement.y == 0 && movement.z == 0) return;
        
        // 카메라 방향 벡터 계산
        float yaw = currentCamera.rotation.y;
        float pitch = currentCamera.rotation.x;
        
        XMFLOAT3 forward = {
            std::sin(yaw) * std::cos(pitch),
            -std::sin(pitch),
            std::cos(yaw) * std::cos(pitch)
        };
        
        XMFLOAT3 right = {
            std::cos(yaw),
            0,
            -std::sin(yaw)
        };
        
        XMFLOAT3 up = {0, 1, 0};
        
        // 최종 이동 벡터
        currentCamera.position.x += movement.x * right.x + movement.z * forward.x;
        currentCamera.position.y += movement.y * up.y + movement.z * forward.y;
        currentCamera.position.z += movement.x * right.z + movement.z * forward.z;
    }
    
    void ProcessMouseLook() {
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        
        if (isFirstMouseInput) {
            lastMousePos = currentMousePos;
            isFirstMouseInput = false;
            return;
        }
        
        int deltaX = currentMousePos.x - lastMousePos.x;
        int deltaY = currentMousePos.y - lastMousePos.y;
        
        if (deltaX != 0 || deltaY != 0) {
            float yawDelta = deltaX * settings.mouseSensitivity * 0.1f;
            float pitchDelta = -deltaY * settings.mouseSensitivity * 0.1f;
            
            currentCamera.rotation.y += yawDelta;
            currentCamera.rotation.x += pitchDelta;
            
            // Pitch 제한
            float maxPitch = XM_PIDIV2 - 0.01f;
            currentCamera.rotation.x = std::max(-maxPitch, std::min(maxPitch, currentCamera.rotation.x));
            
            // Yaw 정규화
            while (currentCamera.rotation.y > XM_2PI) currentCamera.rotation.y -= XM_2PI;
            while (currentCamera.rotation.y < 0) currentCamera.rotation.y += XM_2PI;
        }
        
        lastMousePos = currentMousePos;
    }
    
    void ProcessInput() {
        // 키 상태 업데이트는 입력 스레드에서 처리됨
    }
    
    void ProcessScreenshotQueue() {
        if (screenshotQueue.empty()) return;
        
        // 실제 구현에서는 DirectX/OpenGL을 사용한 스크린캡처
        for (const auto& screenshot : screenshotQueue) {
            // 스크린샷 캡처 및 저장 로직
            CaptureScreenshot(screenshot);
        }
        
        screenshotQueue.clear();
    }
    
    void CaptureScreenshot(const ScreenshotData& data) {
        // 실제 스크린샷 캡처 구현
        // DirectX나 GDI+를 사용하여 화면 캡처
        
        std::wcout << L"스크린샷 저장됨: " << StringToWString(data.filename) << std::endl;
        
        // 메타데이터 저장
        if (data.includeMetadata) {
            SaveScreenshotMetadata(data);
        }
    }
    
    void SaveScreenshotMetadata(const ScreenshotData& data) {
        std::string metaFilename = data.filename + ".meta";
        std::ofstream file(metaFilename);
        
        if (file.is_open()) {
            file << "Camera Position: " << currentCamera.position.x << ", " 
                 << currentCamera.position.y << ", " << currentCamera.position.z << "\n";
            file << "Camera Rotation: " << XMConvertToDegrees(currentCamera.rotation.x) << ", " 
                 << XMConvertToDegrees(currentCamera.rotation.y) << ", " 
                 << XMConvertToDegrees(currentCamera.rotation.z) << "\n";
            file << "FOV: " << XMConvertToDegrees(currentCamera.fov) << "\n";
            file << "Mode: " << GetModeString(currentMode) << "\n";
            file << "Settings: Brightness=" << settings.brightness 
                 << ", Contrast=" << settings.contrast 
                 << ", Saturation=" << settings.saturation << "\n";
            
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            file << "Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
            
            file.close();
        }
    }
    
    void UpdatePerformanceStats() {
        static auto lastTime = std::chrono::high_resolution_clock::now();
        static int frameCount = 0;
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        frameCount++;
        
        auto elapsed = std::chrono::duration<float>(currentTime - lastTime).count();
        if (elapsed >= 1.0f) {
            stats.fps = frameCount / elapsed;
            stats.frameTime = elapsed / frameCount * 1000.0f;  // ms
            
            frameCount = 0;
            lastTime = currentTime;
        }
    }
    
    void InputThreadFunction() {
        while (threadsRunning) {
            UpdateKeyStates();
            ProcessHotkeys();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    void UIThreadFunction() {
        while (threadsRunning) {
            if (isActive && showUI) {
                DisplayUI();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void UpdateKeyStates() {
        previousKeyStates = keyStates;
        
        // 기본 제어 키
        keyStates[VK_F1] = GetAsyncKeyState(VK_F1) & 0x8000;
        keyStates[VK_F2] = GetAsyncKeyState(VK_F2) & 0x8000;
        keyStates[VK_F3] = GetAsyncKeyState(VK_F3) & 0x8000;
        keyStates[VK_F4] = GetAsyncKeyState(VK_F4) & 0x8000;
        keyStates[VK_F5] = GetAsyncKeyState(VK_F5) & 0x8000;
        keyStates[VK_F6] = GetAsyncKeyState(VK_F6) & 0x8000;
        keyStates[VK_F7] = GetAsyncKeyState(VK_F7) & 0x8000;
        keyStates[VK_F8] = GetAsyncKeyState(VK_F8) & 0x8000;
        keyStates[VK_F9] = GetAsyncKeyState(VK_F9) & 0x8000;
        keyStates[VK_F10] = GetAsyncKeyState(VK_F10) & 0x8000;
        keyStates[VK_F11] = GetAsyncKeyState(VK_F11) & 0x8000;
        keyStates[VK_F12] = GetAsyncKeyState(VK_F12) & 0x8000;
        
        // 이동 키
        keyStates['W'] = GetAsyncKeyState('W') & 0x8000;
        keyStates['A'] = GetAsyncKeyState('A') & 0x8000;
        keyStates['S'] = GetAsyncKeyState('S') & 0x8000;
        keyStates['D'] = GetAsyncKeyState('D') & 0x8000;
        keyStates['C'] = GetAsyncKeyState('C') & 0x8000;
        keyStates[VK_SPACE] = GetAsyncKeyState(VK_SPACE) & 0x8000;
        keyStates[VK_SHIFT] = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        keyStates[VK_CONTROL] = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        
        // 마우스
        keyStates[VK_LBUTTON] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        keyStates[VK_RBUTTON] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
        keyStates[VK_MBUTTON] = GetAsyncKeyState(VK_MBUTTON) & 0x8000;
        
        // 기타
        keyStates['H'] = GetAsyncKeyState('H') & 0x8000;
        keyStates['U'] = GetAsyncKeyState('U') & 0x8000;
        keyStates['P'] = GetAsyncKeyState('P') & 0x8000;
        keyStates['R'] = GetAsyncKeyState('R') & 0x8000;
        keyStates['T'] = GetAsyncKeyState('T') & 0x8000;
        keyStates[VK_RETURN] = GetAsyncKeyState(VK_RETURN) & 0x8000;
        keyStates[VK_ESCAPE] = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
        
        // 숫자 키 (북마크 및 모드 전환)
        for (int i = '1'; i <= '9'; i++) {
            keyStates[i] = GetAsyncKeyState(i) & 0x8000;
        }
    }
    
    bool IsKeyPressed(int key) {
        return keyStates[key] && !previousKeyStates[key];
    }
    
    bool IsKeyHeld(int key) {
        return keyStates[key];
    }
    
    void ProcessHotkeys() {
        // F1: 포토 모드 토글
        if (IsKeyPressed(VK_F1)) {
            TogglePhotoMode();
        }
        
        if (!isActive) return;
        
        // F2-F5: 카메라 모드 전환
        if (IsKeyPressed(VK_F2)) SwitchMode(PhotoMode::FreeCam);
        if (IsKeyPressed(VK_F3)) SwitchMode(PhotoMode::OrbitCam);
        if (IsKeyPressed(VK_F4)) SwitchMode(PhotoMode::FixedCam);
        if (IsKeyPressed(VK_F5)) SwitchMode(PhotoMode::CinematicCam);
        
        // H: 도움말 토글
        if (IsKeyPressed('H')) {
            showHelp = !showHelp;
        }
        
        // U: UI 토글
        if (IsKeyPressed('U')) {
            showUI = !showUI;
        }
        
        // P: 스크린샷
        if (IsKeyPressed('P')) {
            TakeScreenshot();
        }
        
        // T: 설정 패널 토글
        if (IsKeyPressed('T')) {
            showSettings = !showSettings;
        }
        
        // R: 원본 상태로 리셋
        if (IsKeyPressed('R')) {
            currentCamera = originalCamera;
            settings = PhotoSettings();
        }
        
        // Escape: 포토 모드 종료
        if (IsKeyPressed(VK_ESCAPE)) {
            DeactivatePhotoMode();
        }
        
        // 숫자 키: 북마크 관련
        for (int i = '1'; i <= '9'; i++) {
            if (IsKeyPressed(i)) {
                int index = i - '1';
                if (IsKeyHeld(VK_SHIFT)) {
                    // Shift + 숫자: 북마크 저장
                    std::string name = "Bookmark_" + std::to_string(index + 1);
                    SaveBookmark(name);
                } else {
                    // 숫자: 북마크 로드
                    LoadBookmark(index);
                }
            }
        }
    }
    
    void DisplayUI() {
        auto now = std::chrono::steady_clock::now();
        if (now - lastUIUpdate < std::chrono::milliseconds(100)) {
            return;
        }
        
        // system("cls"); // Windows 전용, 깜빡임 발생 가능
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord = {0, 0};
        SetConsoleCursorPosition(hConsole, coord);
        
        std::wcout << L"╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::wcout << L"║                        Photo Mode                            ║" << std::endl;
        std::wcout << L"╠══════════════════════════════════════════════════════════════╣" << std::endl;
        
        // 현재 상태
        std::wcout << L"║ Mode: " << std::setw(15) << GetModeString(currentMode) 
                  << L"  FPS: " << std::setw(6) << std::fixed << std::setprecision(1) << stats.fps << L"        ║" << std::endl;
        
        // 카메라 정보
        std::wcout << L"║ Position: (" << std::setw(8) << std::fixed << std::setprecision(2)
                  << currentCamera.position.x << L", " << std::setw(8) << currentCamera.position.y 
                  << L", " << std::setw(8) << currentCamera.position.z << L")   ║" << std::endl;
        
        std::wcout << L"║ Rotation: (" << std::setw(6) << std::fixed << std::setprecision(1)
                  << XMConvertToDegrees(currentCamera.rotation.x) << L"°, " << std::setw(6) 
                  << XMConvertToDegrees(currentCamera.rotation.y) << L"°, " << std::setw(6)
                  << XMConvertToDegrees(currentCamera.rotation.z) << L"°)     ║" << std::endl;
        
        std::wcout << L"║ FOV: " << std::setw(6) << std::fixed << std::setprecision(1)
                  << XMConvertToDegrees(currentCamera.fov) << L"°                                     ║" << std::endl;
        
        std::wcout << L"╠══════════════════════════════════════════════════════════════╣" << std::endl;
        
        // 조작법
        if (showHelp) {
            std::wcout << L"║ Controls:                                                    ║" << std::endl;
            std::wcout << L"║ F1: Toggle Photo Mode    H: Toggle Help                     ║" << std::endl;
            std::wcout << L"║ F2-F5: Camera Modes      U: Toggle UI                      ║" << std::endl;
            std::wcout << L"║ P: Screenshot           T: Settings                         ║" << std::endl;
            std::wcout << L"║ WASD: Move              Space/C: Up/Down                    ║" << std::endl;
            std::wcout << L"║ RClick+Mouse: Look       Wheel: FOV                        ║" << std::endl;
            std::wcout << L"║ 1-9: Load Bookmark      Shift+1-9: Save Bookmark           ║" << std::endl;
            std::wcout << L"║ R: Reset               Esc: Exit Photo Mode                ║" << std::endl;
            std::wcout << L"╠══════════════════════════════════════════════════════════════╣" << std::endl;
        }
        
        // 북마크 목록
        if (!bookmarks.empty()) {
            std::wcout << L"║ Bookmarks:                                                   ║" << std::endl;
            int count = std::min(5, static_cast<int>(bookmarks.size()));
            for (int i = 0; i < count; i++) {
                std::wcout << L"║ " << (i + 1) << L": " << std::setw(25) 
                          << StringToWString(bookmarks[i].name)
                          << L"                              ║" << std::endl;
            }
            std::wcout << L"╠══════════════════════════════════════════════════════════════╣" << std::endl;
        }
        
        // 성능 정보
        std::wcout << L"║ Screenshots: " << std::setw(3) << stats.screenshotCount
                  << L"   Frame Time: " << std::setw(6) << std::fixed << std::setprecision(2) 
                  << stats.frameTime << L"ms           ║" << std::endl;
        
        std::wcout << L"╚══════════════════════════════════════════════════════════════╝" << std::endl;
        
        lastUIUpdate = now;
    }
    
    std::wstring GetModeString(PhotoMode mode) {
        switch (mode) {
            case PhotoMode::FreeCam: return L"Free Camera";
            case PhotoMode::OrbitCam: return L"Orbit Camera";
            case PhotoMode::FixedCam: return L"Fixed Camera";
            case PhotoMode::CinematicCam: return L"Cinematic";
            default: return L"Disabled";
        }
    }
    
    void LoadSettings() {
        std::ifstream file("photo_mode_settings.txt");
        if (file.is_open()) {
            file >> settings.movementSpeed >> settings.rotationSpeed >> settings.mouseSensitivity
                 >> settings.brightness >> settings.contrast >> settings.saturation
                 >> settings.hideUI >> settings.hidePlayer;
            file.close();
        }
    }
    
    void SaveSettings() {
        std::ofstream file("photo_mode_settings.txt");
        if (file.is_open()) {
            file << settings.movementSpeed << " " << settings.rotationSpeed << " " << settings.mouseSensitivity << " "
                 << settings.brightness << " " << settings.contrast << " " << settings.saturation << " "
                 << settings.hideUI << " " << settings.hidePlayer;
            file.close();
        }
    }
    
    void LoadBookmarks() {
        std::ifstream file("photo_bookmarks.txt");
        if (file.is_open()) {
            int count;
            file >> count;
            
            for (int i = 0; i < count; i++) {
                PhotoBookmark bookmark;
                file >> bookmark.name
                     >> bookmark.camera.position.x >> bookmark.camera.position.y >> bookmark.camera.position.z
                     >> bookmark.camera.rotation.x >> bookmark.camera.rotation.y >> bookmark.camera.rotation.z
                     >> bookmark.camera.fov
                     >> bookmark.timestamp >> bookmark.description;
                
                bookmarks.push_back(bookmark);
            }
            
            file.close();
        }
    }
    
    void SaveBookmarks() {
        std::ofstream file("photo_bookmarks.txt");
        if (file.is_open()) {
            file << bookmarks.size() << "\n";
            
            for (const auto& bookmark : bookmarks) {
                file << bookmark.name << " "
                     << bookmark.camera.position.x << " " << bookmark.camera.position.y << " " << bookmark.camera.position.z << " "
                     << bookmark.camera.rotation.x << " " << bookmark.camera.rotation.y << " " << bookmark.camera.rotation.z << " "
                     << bookmark.camera.fov << " "
                     << bookmark.timestamp << " " << bookmark.description << "\n";
            }
            
            file.close();
        }
    }
    
    void PrintControls() {
        std::wcout << L"\n=== 포토 모드 조작법 ===" << std::endl;
        std::wcout << L"F1: 포토 모드 토글" << std::endl;
        std::wcout << L"F2-F5: 카메라 모드 전환" << std::endl;
        std::wcout << L"P: 스크린샷 촬영" << std::endl;
        std::wcout << L"H: 도움말 토글" << std::endl;
        std::wcout << L"U: UI 토글" << std::endl;
        std::wcout << L"\n[카메라 조작]" << std::endl;
        std::wcout << L"WASD: 이동" << std::endl;
        std::wcout << L"Space/C: 위/아래" << std::endl;
        std::wcout << L"우클릭+마우스: 시점 회전" << std::endl;
        std::wcout << L"마우스 휠: FOV 조정" << std::endl;
        std::wcout << L"\n[북마크]" << std::endl;
        std::wcout << L"1-9: 북마크 로드" << std::endl;
        std::wcout << L"Shift+1-9: 북마크 저장" << std::endl;
        std::wcout << L"R: 원본 상태로 리셋" << std::endl;
        std::wcout << L"Esc: 포토 모드 종료" << std::endl;
        std::wcout << L"======================\n" << std::endl;
    }
};

// 정적 멤버 정의
PhotoModeSystem* PhotoModeSystem::instance = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static PhotoModeSystem* system = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"포토 모드 시스템 DLL 로드됨" << std::endl;
            
            system = new PhotoModeSystem();
            if (!system->Initialize()) {
                delete system;
                system = nullptr;
                std::wcout << L"포토 모드 시스템 초기화 실패" << std::endl;
            }
            break;
            
        case DLL_PROCESS_DETACH:
            if (system) {
                system->Shutdown();
                delete system;
                system = nullptr;
            }
            
            FreeConsole();
            break;
    }
    
    return TRUE;
}

// 외부 제어 함수들
extern "C" __declspec(dllexport) void UpdatePhotoMode() {
    PhotoModeSystem* system = PhotoModeSystem::GetInstance();
    if (system) {
        system->Update();
    }
}

extern "C" __declspec(dllexport) void TogglePhotoMode() {
    PhotoModeSystem* system = PhotoModeSystem::GetInstance();
    if (system) {
        system->TogglePhotoMode();
    }
}

extern "C" __declspec(dllexport) void TakeScreenshot() {
    PhotoModeSystem* system = PhotoModeSystem::GetInstance();
    if (system) {
        system->TakeScreenshot();
    }
}

// 독립 실행형 테스트
#ifdef STANDALONE_TEST
int main() {
    std::wcout << L"=== 포토 모드 시스템 테스트 ===" << std::endl;
    
    PhotoModeSystem system;
    
    if (!system.Initialize()) {
        std::wcout << L"초기화 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    std::wcout << L"테스트 시작. 'Q'를 누르면 종료됩니다." << std::endl;
    
    while (true) {
        system.Update();
        
        if (GetAsyncKeyState('Q') & 0x8000) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    system.Shutdown();
    return 0;
}
#endif
