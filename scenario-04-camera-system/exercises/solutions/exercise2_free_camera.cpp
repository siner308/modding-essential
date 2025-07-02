/*
 * Exercise 2: 기본 자유 카메라
 * 
 * 문제: WASD 키로 카메라를 이동하고 마우스로 시점을 변경하는 시스템을 구현하세요.
 * 
 * 학습 목표:
 * - 3D 카메라 이동 구현
 * - 마우스 입력 처리
 * - 부드러운 카메라 제어
 */

#include <Windows.h>
#include <DirectXMath.h>
#include <detours.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <fstream>
#include <map>
#include <cmath> // For std::abs, std::sqrt

#pragma comment(lib, "detours.lib")

using namespace DirectX;

struct CameraData {
    XMFLOAT3 position;      // 0x00: 카메라 위치
    XMFLOAT3 rotation;      // 0x0C: 오일러 각도 (pitch, yaw, roll)
    float fov;              // 0x18: 시야각
    float nearPlane;        // 0x1C: 근거리 클리핑
    float farPlane;         // 0x20: 원거리 클리핑
    float aspectRatio;      // 0x24: 화면 비율
    char padding[8];        // 0x28: 패딩
};

class FreeCameraController {
private:
    static FreeCameraController* instance;
    
    // 카메라 데이터
    uintptr_t cameraAddress;
    CameraData originalCamera;
    CameraData currentCamera;
    bool isInitialized;
    bool isFreeCamEnabled;
    
    // 입력 상태
    std::map<int, bool> keyStates;
    std::map<int, bool> previousKeyStates;
    POINT lastMousePos;
    bool isFirstMouseInput;
    
    // 카메라 설정
    float movementSpeed;
    float rotationSpeed;
    float mouseSensitivity;
    float speedMultiplierFast;
    float speedMultiplierSlow;
    
    // 스레드 관리
    std::thread inputThread;
    std::atomic<bool> inputThreadRunning;
    std::chrono::high_resolution_clock::time_point lastUpdateTime;
    
    // 부드러운 이동을 위한 보간
    struct InterpolationData {
        XMFLOAT3 targetPosition;
        XMFLOAT3 targetRotation;
        bool isInterpolating;
        float interpolationSpeed;
    };
    
    InterpolationData interpolation;

public:
    FreeCameraController() : 
        cameraAddress(0),
        isInitialized(false),
        isFreeCamEnabled(false),
        movementSpeed(5.0f),
        rotationSpeed(90.0f),
        mouseSensitivity(0.1f),
        speedMultiplierFast(3.0f),
        speedMultiplierSlow(0.3f),
        inputThreadRunning(false),
        isFirstMouseInput(true) {
        
        instance = this;
        
        // 초기 마우스 위치 설정
        GetCursorPos(&lastMousePos);
        
        // 보간 초기화
        interpolation.isInterpolating = false;
        interpolation.interpolationSpeed = 5.0f;
    }
    
    ~FreeCameraController() {
        Shutdown();
        instance = nullptr;
    }
    
    static FreeCameraController* GetInstance() {
        return instance;
    }
    
    bool Initialize() {
        std::wcout << L"자유 카메라 컨트롤러 초기화 중..." << std::endl;
        
        // 카메라 주소 찾기
        if (!FindCameraAddress()) {
            std::wcout << L"카메라 주소를 찾을 수 없습니다." << std::endl;
            return false;
        }
        
        // 원본 카메라 상태 백업
        // ReadProcessMemory(GetCurrentProcess(), ... ) 대신 실제 게임 프로세스 핸들 사용 필요
        // 여기서는 예시로 GetCurrentProcess() 사용
        if (!ReadCameraData()) {
            std::wcout << L"카메라 데이터를 읽을 수 없습니다." << std::endl;
            return false;
        }
        
        originalCamera = currentCamera;
        
        // 입력 스레드 시작
        inputThreadRunning = true;
        inputThread = std::thread(&FreeCameraController::InputThreadFunction, this);
        
        isInitialized = true;
        std::wcout << L"자유 카메라 초기화 완료" << std::endl;
        PrintControls();
        
        return true;
    }
    
    void Shutdown() {
        if (!isInitialized) return;
        
        // 입력 스레드 종료
        inputThreadRunning = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
        
        // 원본 카메라 복원
        if (isFreeCamEnabled) {
            RestoreOriginalCamera();
        }
        
        isInitialized = false;
        std::wcout << L"자유 카메라 종료" << std::endl;
    }
    
    void Update() {
        if (!isInitialized) return;
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
        lastUpdateTime = currentTime;
        
        if (isFreeCamEnabled) {
            ProcessInput(deltaTime);
            UpdateInterpolation(deltaTime);
            // WriteProcessMemory(GetCurrentProcess(), ... ) 대신 실제 게임 프로세스 핸들 사용 필요
            WriteCameraData();
        }
    }
    
    void EnableFreeCamera(bool enable) {
        if (!isInitialized) return;
        
        isFreeCamEnabled = enable;
        
        if (enable) {
            std::wcout << L"자유 카메라 모드 활성화" << std::endl;
            currentCamera = originalCamera;  // 현재 상태에서 시작
            isFirstMouseInput = true;
        } else {
            std::wcout << L"자유 카메라 모드 비활성화" << std::endl;
            RestoreOriginalCamera();
        }
    }
    
    bool IsFreeCameraEnabled() const {
        return isFreeCamEnabled;
    }
    
    void SetMovementSpeed(float speed) {
        movementSpeed = speed;
        std::wcout << L"이동 속도 설정: " << speed << std::endl;
    }
    
    void SetMouseSensitivity(float sensitivity) {
        mouseSensitivity = sensitivity;
        std::wcout << L"마우스 감도 설정: " << sensitivity << std::endl;
    }
    
private:
    bool FindCameraAddress() {
        std::wcout << L"카메라 주소 탐색 중..." << std::endl;
        
        // 여기서는 예제로 하드코딩된 주소를 사용
        // 실제로는 패턴 매칭이나 메모리 스캔을 사용해야 함 (exercise1_camera_scanner.cpp 참조)
        cameraAddress = 0x7FF700000000;  // 예제 주소
        
        // 실제 구현에서는 Exercise 1의 결과를 사용하거나
        // 설정 파일에서 주소를 읽어올 수 있음
        std::ifstream configFile("camera_address.txt");
        if (configFile.is_open()) {
            configFile >> std::hex >> cameraAddress;
            configFile.close();
        }
        
        std::wcout << L"카메라 주소: 0x" << std::hex << cameraAddress << std::endl;
        return cameraAddress != 0;
    }
    
    bool ReadCameraData() {
        if (cameraAddress == 0) return false;
        
        SIZE_T bytesRead;
        // ReadProcessMemory(게임프로세스핸들, ...)
        return ReadProcessMemory(GetCurrentProcess(), 
                               reinterpret_cast<LPCVOID>(cameraAddress),
                               &currentCamera, sizeof(CameraData), &bytesRead);
    }
    
    bool WriteCameraData() {
        if (cameraAddress == 0) return false;
        
        SIZE_T bytesWritten;
        // WriteProcessMemory(게임프로세스핸들, ...)
        return WriteProcessMemory(GetCurrentProcess(),
                                reinterpret_cast<LPVOID>(cameraAddress),
                                &currentCamera, sizeof(CameraData), &bytesWritten);
    }
    
    void RestoreOriginalCamera() {
        currentCamera = originalCamera;
        WriteCameraData();
    }
    
    void InputThreadFunction() {
        while (inputThreadRunning) {
            UpdateKeyStates();
            ProcessHotkeys();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }
    
    void UpdateKeyStates() {
        // 이전 상태 저장
        previousKeyStates = keyStates;
        
        // 현재 키 상태 업데이트
        keyStates[VK_LBUTTON] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        keyStates[VK_RBUTTON] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
        keyStates['W'] = GetAsyncKeyState('W') & 0x8000;
        keyStates['A'] = GetAsyncKeyState('A') & 0x8000;
        keyStates['S'] = GetAsyncKeyState('S') & 0x8000;
        keyStates['D'] = GetAsyncKeyState('D') & 0x8000;
        keyStates['Q'] = GetAsyncKeyState('Q') & 0x8000;
        keyStates['E'] = GetAsyncKeyState('E') & 0x8000;
        keyStates[VK_SPACE] = GetAsyncKeyState(VK_SPACE) & 0x8000;
        keyStates['C'] = GetAsyncKeyState('C') & 0x8000;
        keyStates[VK_SHIFT] = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        keyStates[VK_CONTROL] = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        keyStates[VK_F1] = GetAsyncKeyState(VK_F1) & 0x8000;
        keyStates[VK_F2] = GetAsyncKeyState(VK_F2) & 0x8000;
        keyStates[VK_F3] = GetAsyncKeyState(VK_F3) & 0x8000;
        keyStates[VK_F4] = GetAsyncKeyState(VK_F4) & 0x8000;
    }
    
    bool IsKeyPressed(int key) {
        return keyStates[key] && !previousKeyStates[key];
    }
    
    bool IsKeyHeld(int key) {
        return keyStates[key];
    }
    
    void ProcessHotkeys() {
        // F1: 자유 카메라 토글
        if (IsKeyPressed(VK_F1)) {
            EnableFreeCamera(!isFreeCamEnabled);
        }
        
        // F2: 원본 카메라 복원
        if (IsKeyPressed(VK_F2)) {
            if (isFreeCamEnabled) {
                currentCamera = originalCamera;
                std::wcout << L"원본 카메라 위치로 복원" << std::endl;
            }
        }
        
        // F3: 현재 위치 저장
        if (IsKeyPressed(VK_F3)) {
            if (isFreeCamEnabled) {
                SaveCurrentPosition();
            }
        }
        
        // F4: 저장된 위치 로드
        if (IsKeyPressed(VK_F4)) {
            if (isFreeCamEnabled) {
                LoadSavedPosition();
            }
        }
    }
    
    void ProcessInput(float deltaTime) {
        if (!isFreeCamEnabled) return;
        
        // 마우스 입력 처리
        ProcessMouseInput();
        
        // 키보드 이동 입력 처리
        ProcessMovementInput(deltaTime);
    }
    
    void ProcessMouseInput() {
        if (!IsKeyHeld(VK_RBUTTON)) return;  // 우클릭 시에만 마우스 룩
        
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        
        if (isFirstMouseInput) {
            lastMousePos = currentMousePos;
            isFirstMouseInput = false;
            return;
        }
        
        // 마우스 델타 계산
        int deltaX = currentMousePos.x - lastMousePos.x;
        int deltaY = currentMousePos.y - lastMousePos.y;
        
        if (deltaX != 0 || deltaY != 0) {
            // 회전 적용
            float yawDelta = deltaX * mouseSensitivity * XM_PI / 180.0f;
            float pitchDelta = -deltaY * mouseSensitivity * XM_PI / 180.0f;
            
            currentCamera.rotation.y += yawDelta;  // Yaw
            currentCamera.rotation.x += pitchDelta; // Pitch
            
            // Pitch 제한 (-90도 ~ +90도)
            float maxPitch = XM_PIDIV2 - 0.01f;
            currentCamera.rotation.x = std::max(-maxPitch, std::min(maxPitch, currentCamera.rotation.x));
            
            // Yaw를 0~2π 범위로 정규화
            while (currentCamera.rotation.y > XM_2PI) currentCamera.rotation.y -= XM_2PI;
            while (currentCamera.rotation.y < 0) currentCamera.rotation.y += XM_2PI;
            
            // 마우스를 중앙으로 다시 설정
            SetCursorPos(lastMousePos.x, lastMousePos.y);
        }
    }
    
    void ProcessMovementInput(float deltaTime) {
        XMFLOAT3 movement = {0, 0, 0};
        
        // 속도 조절
        float speed = movementSpeed;
        if (IsKeyHeld(VK_SHIFT)) speed *= speedMultiplierFast;
        if (IsKeyHeld(VK_CONTROL)) speed *= speedMultiplierSlow;
        
        // 이동 입력
        if (IsKeyHeld('W')) movement.z += speed * deltaTime;  // 앞으로
        if (IsKeyHeld('S')) movement.z -= speed * deltaTime;  // 뒤로
        if (IsKeyHeld('A')) movement.x -= speed * deltaTime;  // 왼쪽
        if (IsKeyHeld('D')) movement.x += speed * deltaTime;  // 오른쪽
        if (IsKeyHeld(VK_SPACE) || IsKeyHeld('E')) movement.y += speed * deltaTime;  // 위
        if (IsKeyHeld('C') || IsKeyHeld('Q')) movement.y -= speed * deltaTime;  // 아래
        
        // 카메라 방향 기준으로 이동 벡터 변환
        ApplyMovement(movement);
    }
    
    void ApplyMovement(const XMFLOAT3& localMovement) {
        if (localMovement.x == 0 && localMovement.y == 0 && localMovement.z == 0) {
            return;
        }
        
        // 카메라 방향 벡터들 계산
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
        
        // 위쪽 방향 (월드 업 벡터)
        XMFLOAT3 up = {0, 1, 0};
        
        // 최종 이동 벡터 계산
        XMFLOAT3 worldMovement = {
            localMovement.x * right.x + localMovement.z * forward.x,
            localMovement.y * up.y + localMovement.z * forward.y,
            localMovement.x * right.z + localMovement.z * forward.z
        };
        
        // 위치 업데이트
        currentCamera.position.x += worldMovement.x;
        currentCamera.position.y += worldMovement.y;
        currentCamera.position.z += worldMovement.z;
    }
    
    void UpdateInterpolation(float deltaTime) {
        if (!interpolation.isInterpolating) return;
        
        float t = interpolation.interpolationSpeed * deltaTime;
        t = std::min(t, 1.0f);
        
        // 위치 보간
        currentCamera.position.x = Lerp(currentCamera.position.x, interpolation.targetPosition.x, t);
        currentCamera.position.y = Lerp(currentCamera.position.y, interpolation.targetPosition.y, t);
        currentCamera.position.z = Lerp(currentCamera.position.z, interpolation.targetPosition.z, t);
        
        // 회전 보간 (각도를 고려한 보간)
        currentCamera.rotation.x = LerpAngle(currentCamera.rotation.x, interpolation.targetRotation.x, t);
        currentCamera.rotation.y = LerpAngle(currentCamera.rotation.y, interpolation.targetRotation.y, t);
        currentCamera.rotation.z = LerpAngle(currentCamera.rotation.z, interpolation.targetRotation.z, t);
        
        // 보간 완료 확인
        float positionDistance = Distance(currentCamera.position, interpolation.targetPosition);
        float rotationDistance = std::abs(currentCamera.rotation.x - interpolation.targetRotation.x) +
                                std::abs(currentCamera.rotation.y - interpolation.targetRotation.y) +
                                std::abs(currentCamera.rotation.z - interpolation.targetRotation.z);
        
        if (positionDistance < 0.01f && rotationDistance < 0.01f) {
            interpolation.isInterpolating = false;
        }
    }
    
    float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    float LerpAngle(float a, float b, float t) {
        // 최단 경로로 각도 보간
        float diff = b - a;
        while (diff > XM_PI) diff -= XM_2PI;
        while (diff < -XM_PI) diff += XM_2PI;
        return a + diff * t;
    }
    
    float Distance(const XMFLOAT3& a, const XMFLOAT3& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    void SaveCurrentPosition() {
        std::ofstream file("camera_position.txt");
        if (file.is_open()) {
            file << currentCamera.position.x << " " << currentCamera.position.y << " " << currentCamera.position.z << "\n";
            file << currentCamera.rotation.x << " " << currentCamera.rotation.y << " " << currentCamera.rotation.z << "\n";
            file.close();
            std::wcout << L"현재 카메라 위치 저장됨" << std::endl;
        }
    }
    
    void LoadSavedPosition() {
        std::ifstream file("camera_position.txt");
        if (file.is_open()) {
            XMFLOAT3 savedPosition, savedRotation;
            file >> savedPosition.x >> savedPosition.y >> savedPosition.z;
            file >> savedRotation.x >> savedRotation.y >> savedRotation.z;
            file.close();
            
            // 부드러운 이동 시작
            interpolation.targetPosition = savedPosition;
            interpolation.targetRotation = savedRotation;
            interpolation.isInterpolating = true;
            
            std::wcout << L"저장된 카메라 위치로 이동 중..." << std::endl;
        } else {
            std::wcout << L"저장된 위치를 찾을 수 없습니다." << std::endl;
        }
    }
    
    void PrintControls() {
        std::wcout << L"\n=== 자유 카메라 조작법 ===" << std::endl;
        std::wcout << L"F1: 자유 카메라 토글" << std::endl;
        std::wcout << L"F2: 원본 위치 복원" << std::endl;
        std::wcout << L"F3: 현재 위치 저장" << std::endl;
        std::wcout << L"F4: 저장된 위치 로드" << std::endl;
        std::wcout << L"\n[자유 카메라 모드]" << std::endl;
        std::wcout << L"우클릭 + 마우스: 시점 회전" << std::endl;
        std::wcout << L"WASD: 평면 이동" << std::endl;
        std::wcout << L"Space/E: 위로 이동" << std::endl;
        std::wcout << L"C/Q: 아래로 이동" << std::endl;
        std::wcout << L"Shift: 빠른 이동" << std::endl;
        std::wcout << L"Ctrl: 느린 이동" << std::endl;
        std::wcout << L"========================\n" << std::endl;
    }
};

// 정적 멤버 정의
FreeCameraController* FreeCameraController::instance = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static FreeCameraController* controller = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"자유 카메라 DLL 로드됨" << std::endl;
            
            controller = new FreeCameraController();
            if (!controller->Initialize()) {
                delete controller;
                controller = nullptr;
                std::wcout << L"자유 카메라 초기화 실패" << std::endl;
            }
            break;
            
        case DLL_PROCESS_DETACH:
            if (controller) {
                controller->Shutdown();
                delete controller;
                controller = nullptr;
            }
            
            FreeConsole();
            break;
    }
    
    return TRUE;
}

// 업데이트 루프 (메인 스레드에서 호출)
extern "C" __declspec(dllexport) void UpdateFreeCamera() {
    FreeCameraController* controller = FreeCameraController::GetInstance();
    if (controller) {
        controller->Update();
    }
}

// 외부 제어 함수들
extern "C" __declspec(dllexport) void EnableFreeCamera(bool enable) {
    FreeCameraController* controller = FreeCameraController::GetInstance();
    if (controller) {
        controller->EnableFreeCamera(enable);
    }
}

extern "C" __declspec(dllexport) bool IsFreeCameraEnabled() {
    FreeCameraController* controller = FreeCameraController::GetInstance();
    return controller ? controller->IsFreeCameraEnabled() : false;
}

extern "C" __declspec(dllexport) void SetCameraSpeed(float speed) {
    FreeCameraController* controller = FreeCameraController::GetInstance();
    if (controller) {
        controller->SetMovementSpeed(speed);
    }
}

extern "C" __declspec(dllexport) void SetMouseSensitivity(float sensitivity) {
    FreeCameraController* controller = FreeCameraController::GetInstance();
    if (controller) {
        controller->SetMouseSensitivity(sensitivity);
    }
}

// 독립 실행형 테스트 (콘솔 애플리케이션)
#ifdef STANDALONE_TEST
int main() {
    std::wcout << L"=== 자유 카메라 테스트 ===" << std::endl;
    
    FreeCameraController controller;
    
    if (!controller.Initialize()) {
        std::wcout << L"초기화 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    std::wcout << L"테스트 시작. 'q'를 누르면 종료됩니다." << std::endl;
    
    // 메인 루프
    while (true) {
        controller.Update();
        
        // 종료 확인
        if (GetAsyncKeyState('Q') & 0x8000) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    controller.Shutdown();
    std::wcout << L"테스트 종료" << std::endl;
    
    return 0;
}
#endif