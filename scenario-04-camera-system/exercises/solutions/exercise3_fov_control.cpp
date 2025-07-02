/*
 * Exercise 3: FOV 조정
 * 
 * 문제: 마우스 휠이나 키보드로 실시간 FOV 조정이 가능한 시스템을 만드세요.
 * 
 * 학습 목표:
 * - FOV 계산 및 적용
 * - 실시간 파라미터 조정
 * - 사용자 친화적 인터페이스
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
#include <vector>
#include <map>
#include <iomanip>
#include <cmath> // For std::abs, std::min, std::max
#include <limits> // For std::numeric_limits

#pragma comment(lib, "detours.lib")

using namespace DirectX;

struct CameraData {
    XMFLOAT3 position;      // 0x00: 카메라 위치
    XMFLOAT3 rotation;      // 0x0C: 오일러 각도
    float fov;              // 0x18: 시야각 (radians)
    float nearPlane;        // 0x1C: 근거리 클리핑
    float farPlane;         // 0x20: 원거리 클리핑
    float aspectRatio;      // 0x24: 화면 비율
    char padding[8];        // 0x28: 패딩
};

class FOVController {
private:
    static FOVController* instance;
    
    // 카메라 데이터
    uintptr_t cameraAddress;
    CameraData originalCamera;
    CameraData currentCamera;
    bool isInitialized;
    bool isEnabled;
    
    // FOV 설정
    float currentFOV;           // 현재 FOV (도 단위)
    float originalFOV;          // 원본 FOV
    float minFOV;              // 최소 FOV
    float maxFOV;              // 최대 FOV
    float fovStep;             // FOV 조정 단위
    bool smoothTransition;      // 부드러운 전환
    float transitionSpeed;      // 전환 속도
    
    // 프리셋 시스템
    struct FOVPreset {
        std::string name;
        float fov;
        std::string description;
    };
    
    std::vector<FOVPreset> presets;
    int currentPresetIndex;
    
    // 입력 처리
    std::map<int, bool> keyStates;
    std::map<int, bool> previousKeyStates;
    int mouseWheelDelta;
    
    // 스레드 관리
    std::thread inputThread;
    std::atomic<bool> inputThreadRunning;
    
    // UI 표시
    bool showUI;
    std::chrono::steady_clock::time_point lastUIToggleTime;
    std::chrono::steady_clock::time_point lastFOVChangeTime;
    
    // 통계 및 모니터링
    struct FOVStats {
        float averageFOV;
        int adjustmentCount;
        std::chrono::steady_clock::time_point sessionStart;
        std::vector<std::pair<std::chrono::steady_clock::time_point, float>> fovHistory;
    };
    
    FOVStats stats;

public:
    FOVController() : 
        cameraAddress(0),
        isInitialized(false),
        isEnabled(false),
        currentFOV(90.0f),
        originalFOV(90.0f),
        minFOV(10.0f),
        maxFOV(179.0f),
        fovStep(5.0f),
        smoothTransition(true),
        transitionSpeed(5.0f),
        currentPresetIndex(0),
        mouseWheelDelta(0),
        inputThreadRunning(false),
        showUI(true) {
        
        instance = this;
        
        // 기본 프리셋 초기화
        InitializePresets();
        
        // 통계 초기화
        stats.adjustmentCount = 0;
        stats.sessionStart = std::chrono::steady_clock::now();
    }
    
    ~FOVController() {
        Shutdown();
        instance = nullptr;
    }
    
    static FOVController* GetInstance() {
        return instance;
    }
    
    bool Initialize() {
        std::wcout << L"FOV 컨트롤러 초기화 중..." << std::endl;
        
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
        originalFOV = XMConvertToDegrees(currentCamera.fov);
        currentFOV = originalFOV;
        
        // 입력 스레드 시작
        inputThreadRunning = true;
        inputThread = std::thread(&FOVController::InputThreadFunction, this);
        
        // 설정 파일 로드
        LoadSettings();
        
        isInitialized = true;
        std::wcout << L"FOV 컨트롤러 초기화 완료" << std::endl;
        std::wcout << L"원본 FOV: " << std::fixed << std::setprecision(1) << originalFOV << L"도" << std::endl;
        PrintControls();
        
        return true;
    }
    
    void Shutdown() {
        if (!isInitialized) return;
        
        // 설정 저장
        SaveSettings();
        
        // 입력 스레드 종료
        inputThreadRunning = false;
        if (inputThread.joinable()) {
            inputThread.join();
        }
        
        // 원본 FOV 복원
        if (isEnabled) {
            RestoreOriginalFOV();
        }
        
        isInitialized = false;
        std::wcout << L"FOV 컨트롤러 종료" << std::endl;
    }
    
    void Update() {
        if (!isInitialized) return;
        
        if (isEnabled) {
            ProcessInput();
            UpdateFOVTransition();
            // WriteProcessMemory(GetCurrentProcess(), ... ) 대신 실제 게임 프로세스 핸들 사용 필요
            WriteCameraData();
            
            if (showUI) {
                DisplayUI();
            }
        }
    }
    
    void Enable(bool enable) {
        if (!isInitialized) return;
        
        isEnabled = enable;
        
        if (enable) {
            std::wcout << L"FOV 조정 모드 활성화" << std::endl;
        } else {
            std::wcout << L"FOV 조정 모드 비활성화" << std::endl;
            RestoreOriginalFOV();
        }
    }
    
    void SetFOV(float fov, bool instant = false) {
        fov = std::max(minFOV, std::min(maxFOV, fov));
        
        if (instant || !smoothTransition) {
            currentFOV = fov;
            currentCamera.fov = XMConvertToRadians(fov);
        } else {
            // 부드러운 전환을 위해 타겟 FOV만 설정
            targetFOV = fov;
            isTransitioning = true;
        }
        
        // 통계 업데이트
        stats.adjustmentCount++;
        stats.fovHistory.push_back({std::chrono::steady_clock::now(), fov});
        lastFOVChangeTime = std::chrono::steady_clock::now();
        
        std::wcout << L"FOV 설정: " << std::fixed << std::setprecision(1) << fov << L"도" << std::endl;
    }
    
    void AdjustFOV(float delta) {
        SetFOV(currentFOV + delta);
    }
    
    void SetPreset(int index) {
        if (index >= 0 && index < presets.size()) {
            currentPresetIndex = index;
            SetFOV(presets[index].fov);
            std::wcout << L"프리셋 적용: " << std::wstring(presets[index].name.begin(), presets[index].name.end()) 
                      << L" (" << presets[index].fov << L"도)" << std::endl;
        }
    }
    
    void NextPreset() {
        currentPresetIndex = (currentPresetIndex + 1) % presets.size();
        SetPreset(currentPresetIndex);
    }
    
    void PreviousPreset() {
        currentPresetIndex = (currentPresetIndex - 1 + presets.size()) % presets.size();
        SetPreset(currentPresetIndex);
    }
    
    void AddCustomPreset(const std::string& name, float fov) {
        FOVPreset preset;
        preset.name = name;
        preset.fov = fov;
        preset.description = "Custom preset";
        presets.push_back(preset);
        
        std::wcout << L"커스텀 프리셋 추가: " << std::wstring(name.begin(), name.end()) 
                  << L" (" << fov << L"도)" << std::endl;
    }
    
    float GetCurrentFOV() const {
        return currentFOV;
    }
    
    void ToggleUI() {
        auto now = std::chrono::steady_clock::now();
        if (now - lastUIToggleTime > std::chrono::milliseconds(200)) {
            showUI = !showUI;
            lastUIToggleTime = now;
        }
    }

private:
    float targetFOV;
    bool isTransitioning;
    
    void InitializePresets() {
        presets = {
            {"Narrow", 30.0f, "Telephoto effect"},
            {"Cinematic", 50.0f, "Movie-like view"},
            {"Normal", 75.0f, "Standard gaming FOV"},
            {"Wide", 90.0f, "Default wide view"},
            {"Ultra Wide", 110.0f, "Immersive wide angle"},
            {"Fisheye", 150.0f, "Extreme wide angle"}
        };
    }
    
    bool FindCameraAddress() {
        // 실제 구현에서는 패턴 매칭이나 메모리 스캔 사용
        cameraAddress = 0x7FF700000000;  // 예제 주소
        
        // 설정 파일에서 주소 읽기
        std::ifstream configFile("camera_address.txt");
        if (configFile.is_open()) {
            configFile >> std::hex >> cameraAddress;
            configFile.close();
        }
        
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
    
    void RestoreOriginalFOV() {
        currentCamera.fov = originalCamera.fov;
        currentFOV = originalFOV;
        WriteCameraData();
    }
    
    void InputThreadFunction() {
        while (inputThreadRunning) {
            UpdateKeyStates();
            ProcessHotkeys();
            ProcessMouseWheel();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    void UpdateKeyStates() {
        previousKeyStates = keyStates;
        
        keyStates[VK_F5] = GetAsyncKeyState(VK_F5) & 0x8000;
        keyStates[VK_F6] = GetAsyncKeyState(VK_F6) & 0x8000;
        keyStates[VK_F7] = GetAsyncKeyState(VK_F7) & 0x8000;
        keyStates[VK_F8] = GetAsyncKeyState(VK_F8) & 0x8000;
        keyStates[VK_F9] = GetAsyncKeyState(VK_F9) & 0x8000;
        keyStates[VK_F10] = GetAsyncKeyState(VK_F10) & 0x8000;
        keyStates[VK_PRIOR] = GetAsyncKeyState(VK_PRIOR) & 0x8000;  // Page Up
        keyStates[VK_NEXT] = GetAsyncKeyState(VK_NEXT) & 0x8000;    // Page Down
        keyStates[VK_UP] = GetAsyncKeyState(VK_UP) & 0x8000;
        keyStates[VK_DOWN] = GetAsyncKeyState(VK_DOWN) & 0x8000;
        keyStates[VK_LEFT] = GetAsyncKeyState(VK_LEFT) & 0x8000;
        keyStates[VK_RIGHT] = GetAsyncKeyState(VK_RIGHT) & 0x8000;
        keyStates[VK_CONTROL] = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        keyStates[VK_SHIFT] = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        keyStates['H'] = GetAsyncKeyState('H') & 0x8000;
        
        // 숫자 키 1-6 (프리셋 선택용)
        for (int i = '1'; i <= '6'; i++) {
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
        // F5: FOV 조정 모드 토글
        if (IsKeyPressed(VK_F5)) {
            Enable(!isEnabled);
        }
        
        // F6: 원본 FOV 복원
        if (IsKeyPressed(VK_F6)) {
            if (isEnabled) {
                SetFOV(originalFOV, true);
            }
        }
        
        // F7: 다음 프리셋
        if (IsKeyPressed(VK_F7)) {
            if (isEnabled) {
                NextPreset();
            }
        }
        
        // F8: 이전 프리셋
        if (IsKeyPressed(VK_F8)) {
            if (isEnabled) {
                PreviousPreset();
            }
        }
        
        // F9: 현재 FOV를 커스텀 프리셋으로 저장
        if (IsKeyPressed(VK_F9)) {
            if (isEnabled) {
                std::string name = "Custom_" + std::to_string(currentFOV);
                AddCustomPreset(name, currentFOV);
            }
        }
        
        // F10: 통계 표시
        if (IsKeyPressed(VK_F10)) {
            if (isEnabled) {
                ShowStatistics();
            }
        }
        
        // H: UI 토글
        if (IsKeyPressed('H')) {
            if (isEnabled) {
                ToggleUI();
            }
        }
        
        // 숫자 키로 프리셋 직접 선택
        for (int i = '1'; i <= '6'; i++) {
            if (IsKeyPressed(i)) {
                if (isEnabled) {
                    SetPreset(i - '1');
                }
            }
        }
    }
    
    void ProcessInput() {
        if (!isEnabled) return;
        
        float delta = 0.0f;
        float stepSize = fovStep;
        
        // 미세 조정 모드
        if (IsKeyHeld(VK_CONTROL)) {
            stepSize = 1.0f;
        }
        // 빠른 조정 모드
        else if (IsKeyHeld(VK_SHIFT)) {
            stepSize = 10.0f;
        }
        
        // 키보드 입력
        if (IsKeyHeld(VK_PRIOR) || IsKeyHeld(VK_UP)) {
            delta = stepSize;
        }
        if (IsKeyHeld(VK_NEXT) || IsKeyHeld(VK_DOWN)) {
            delta = -stepSize;
        }
        
        if (delta != 0.0f) {
            AdjustFOV(delta);
        }
    }
    
    void ProcessMouseWheel() {
        // 실제 구현에서는 윈도우 메시지나 후킹을 통해 마우스 휠 델타 획득
        // 여기서는 시뮬레이션
        if (mouseWheelDelta != 0 && isEnabled) {
            float delta = mouseWheelDelta > 0 ? fovStep : -fovStep;
            
            // Ctrl 키로 미세 조정
            if (IsKeyHeld(VK_CONTROL)) {
                delta *= 0.2f;
            }
            
            AdjustFOV(delta);
            mouseWheelDelta = 0;
        }
    }
    
    void UpdateFOVTransition() {
        if (!isTransitioning) return;
        
        float deltaTime = 0.016f;  // ~60 FPS 가정
        float speed = transitionSpeed * deltaTime;
        
        float diff = targetFOV - currentFOV;
        if (std::abs(diff) < 0.1f) {
            currentFOV = targetFOV;
            isTransitioning = false;
        } else {
            currentFOV += diff * speed;
        }
        
        currentCamera.fov = XMConvertToRadians(currentFOV);
    }
    
    void DisplayUI() {
        // 콘솔에 간단한 UI 표시
        static auto lastDisplayTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        
        if (now - lastDisplayTime > std::chrono::milliseconds(100)) {
            // system("cls");  // Windows 전용, 깜빡임 발생 가능
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD coord = {0, 0};
            SetConsoleCursorPosition(hConsole, coord);
            
            std::wcout << L"╔════════════════════════════════════════╗" << std::endl;
            std::wcout << L"║              FOV Controller            ║" << std::endl;
            std::wcout << L"╠════════════════════════════════════════╣" << std::endl;
            std::wcout << L"║ Current FOV: " << std::setw(6) << std::fixed << std::setprecision(1) 
                      << currentFOV << L"°                   ║" << std::endl;
            std::wcout << L"║ Original FOV: " << std::setw(5) << std::fixed << std::setprecision(1) 
                      << originalFOV << L"°                   ║" << std::endl;
            std::wcout << L"║ Range: " << std::setw(3) << minFOV << L"° - " << std::setw(3) << maxFOV << L"°              ║" << std::endl;
            std::wcout << L"╠════════════════════════════════════════╣" << std::endl;
            std::wcout << L"║ Current Preset: " << std::setw(2) << (currentPresetIndex + 1) << L"/6           ║" << std::endl;
            std::wcout << L"║ " << std::setw(15) << std::wstring(presets[currentPresetIndex].name.begin(), 
                                                              presets[currentPresetIndex].name.end()) 
                      << L" (" << std::setw(3) << presets[currentPresetIndex].fov << L"°)        ║" << std::endl;
            std::wcout << L"╠════════════════════════════════════════╣" << std::endl;
            std::wcout << L"║ Controls:                              ║" << std::endl;
            std::wcout << L"║ PageUp/Down: Adjust FOV                ║" << std::endl;
            std::wcout << L"║ F7/F8: Next/Prev Preset               ║" << std::endl;
            std::wcout << L"║ 1-6: Select Preset                    ║" << std::endl;
            std::wcout << L"║ Ctrl: Fine adjustment                  ║" << std::endl;
            std::wcout << L"║ H: Toggle UI                           ║" << std::endl;
            std::wcout << L"╚════════════════════════════════════════╝" << std::endl;
            
            // FOV 바 표시
            int barWidth = 30;
            float percentage = (currentFOV - minFOV) / (maxFOV - minFOV);
            int filledWidth = static_cast<int>(percentage * barWidth);
            
            std::wcout << L"FOV: [";
            for (int i = 0; i < barWidth; i++) {
                if (i < filledWidth) {
                    std::wcout << L"█";
                } else {
                    std::wcout << L"░";
                }
            }
            std::wcout << L"]" << std::endl;
            
            lastDisplayTime = now;
        }
    }
    
    void ShowStatistics() {
        auto now = std::chrono::steady_clock::now();
        auto sessionDuration = std::chrono::duration_cast<std::chrono::minutes>(now - stats.sessionStart);
        
        std::wcout << L"\n=== FOV 사용 통계 ===" << std::endl;
        std::wcout << L"세션 시간: " << sessionDuration.count() << L"분" << std::endl;
        std::wcout << L"조정 횟수: " << stats.adjustmentCount << std::endl;
        std::wcout << L"현재 FOV: " << std::fixed << std::setprecision(1) << currentFOV << L"도" << std::endl;
        std::wcout << L"원본 FOV: " << std::fixed << std::setprecision(1) << originalFOV << L"도" << std::endl;
        
        // 최근 FOV 변경 이력
        if (stats.fovHistory.size() > 0) {
            std::wcout << L"최근 변경 이력:" << std::endl;
            int count = std::min(5, static_cast<int>(stats.fovHistory.size()));
            for (int i = stats.fovHistory.size() - count; i < stats.fovHistory.size(); i++) {
                std::wcout << L"  " << std::fixed << std::setprecision(1) 
                          << stats.fovHistory[i].second << L"도" << std::endl;
            }
        }
        
        std::wcout << L"==================" << std::endl;
    }
    
    void LoadSettings() {
        std::ifstream file("fov_settings.txt");
        if (file.is_open()) {
            file >> minFOV >> maxFOV >> fovStep >> smoothTransition >> transitionSpeed;
            file.close();
            std::wcout << L"설정 로드 완료" << std::endl;
        }
    }
    
    void SaveSettings() {
        std::ofstream file("fov_settings.txt");
        if (file.is_open()) {
            file << minFOV << " " << maxFOV << " " << fovStep << " " << smoothTransition << " " << transitionSpeed;
            file.close();
            std::wcout << L"설정 저장 완료" << std::endl;
        }
    }
    
    void PrintControls() {
        std::wcout << L"\n=== FOV 컨트롤 조작법 ===" << std::endl;
        std::wcout << L"F5: FOV 조정 모드 토글" << std::endl;
        std::wcout << L"F6: 원본 FOV 복원" << std::endl;
        std::wcout << L"F7/F8: 다음/이전 프리셋" << std::endl;
        std::wcout << L"F9: 현재 FOV를 프리셋으로 저장" << std::endl;
        std::wcout << L"F10: 통계 표시" << std::endl;
        std::wcout << L"H: UI 토글" << std::endl;
        std::wcout << L"\n[FOV 조정 모드]" << std::endl;
        std::wcout << L"PageUp/Down, ↑/↓: FOV 조정" << std::endl;
        std::wcout << L"1-6: 프리셋 선택" << std::endl;
        std::wcout << L"Ctrl: 미세 조정" << std::endl;
        std::wcout << L"Shift: 빠른 조정" << std::endl;
        std::wcout << L"마우스 휠: FOV 조정" << std::endl;
        std::wcout << L"========================\n" << std::endl;
    }
};

// 정적 멤버 정의
FOVController* FOVController::instance = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static FOVController* controller = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"FOV 컨트롤러 DLL 로드됨" << std::endl;
            
            controller = new FOVController();
            if (!controller->Initialize()) {
                delete controller;
                controller = nullptr;
                std::wcout << L"FOV 컨트롤러 초기화 실패" << std::endl;
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

// 외부 제어 함수들
extern "C" __declspec(dllexport) void UpdateFOVController() {
    FOVController* controller = FOVController::GetInstance();
    if (controller) {
        controller->Update();
    }
}

extern "C" __declspec(dllexport) void EnableFOVControl(bool enable) {
    FOVController* controller = FOVController::GetInstance();
    if (controller) {
        controller->Enable(enable);
    }
}

extern "C" __declspec(dllexport) void SetFOV(float fov) {
    FOVController* controller = FOVController::GetInstance();
    if (controller) {
        controller->SetFOV(fov);
    }
}

extern "C" __declspec(dllexport) float GetCurrentFOV() {
    FOVController* controller = FOVController::GetInstance();
    return controller ? controller->GetCurrentFOV() : 90.0f;
}

extern "C" __declspec(dllexport) void SetFOVPreset(int index) {
    FOVController* controller = FOVController::GetInstance();
    if (controller) {
        controller->SetPreset(index);
    }
}

// 독립 실행형 테스트
#ifdef STANDALONE_TEST
int main() {
    std::wcout << L"=== FOV 컨트롤러 테스트 ===" << std::endl;
    
    FOVController controller;
    
    if (!controller.Initialize()) {
        std::wcout << L"초기화 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    controller.Enable(true);
    
    std::wcout << L"테스트 시작. 'Q'를 누르면 종료됩니다." << std::endl;
    
    while (true) {
        controller.Update();
        
        if (GetAsyncKeyState('Q') & 0x8000) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    controller.Shutdown();
    return 0;
}
#endif