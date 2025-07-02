/*
 * Exercise 4: 카메라 애니메이션
 * 
 * 문제: 두 지점 사이를 부드럽게 이동하는 카메라 전환 시스템을 구현하세요.
 * 
 * 학습 목표:
 * - 카메라 애니메이션 시스템
 * - 이징 함수 구현
 * - 키프레임 시스템
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
#include <algorithm>
#include <functional>
#include <cmath> // For std::abs, std::min, std::max, std::cos, std::sin, std::pow
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

enum class EaseType {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic
};

struct CameraKeyframe {
    float time;
    CameraData cameraState;
    EaseType easeType;
    std::string name;
    
    CameraKeyframe() : time(0.0f), easeType(EaseType::Linear) {}
    
    CameraKeyframe(float t, const CameraData& state, EaseType ease = EaseType::Linear, const std::string& n = "") 
        : time(t), cameraState(state), easeType(ease), name(n) {}
};

class CameraTransitionSystem {
private:
    static CameraTransitionSystem* instance;
    
    // 카메라 데이터
    uintptr_t cameraAddress;
    CameraData originalCamera;
    CameraData currentCamera;
    bool isInitialized;
    bool isEnabled;
    
    // 키프레임 시스템
    std::vector<CameraKeyframe> keyframes;
    bool isPlaying;
    float currentTime;
    float totalDuration;
    bool isLooping;
    int playbackSpeed;  // 0.5x, 1x, 2x 등
    
    // 단순 전환 시스템
    struct SimpleTransition {
        CameraData startState;
        CameraData endState;
        float duration;
        float elapsed;
        EaseType easeType;
        bool isActive;
        std::function<void()> onComplete;
    };
    
    SimpleTransition currentTransition;
    
    // 입력 및 제어
    std::map<int, bool> keyStates;
    std::map<int, bool> previousKeyStates;
    std::thread inputThread;
    std::atomic<bool> inputThreadRunning;
    
    // 시간 관리
    std::chrono::high_resolution_clock::time_point lastUpdateTime;
    float deltaTime;
    
    // UI 및 상태
    bool showUI;
    bool showPreview;
    int selectedKeyframe;
    
    // 사전 정의된 애니메이션
    struct AnimationPreset {
        std::string name;
        std::vector<CameraKeyframe> keyframes;
        std::string description;
    };
    
    std::vector<AnimationPreset> presets;
    
    // 통계
    struct PlaybackStats {
        int totalTransitions;
        float totalPlayTime;
        std::chrono::steady_clock::time_point sessionStart;
    };
    
    PlaybackStats stats;

public:
    CameraTransitionSystem() : 
        cameraAddress(0),
        isInitialized(false),
        isEnabled(false),
        isPlaying(false),
        currentTime(0.0f),
        totalDuration(0.0f),
        isLooping(false),
        playbackSpeed(1),
        inputThreadRunning(false),
        deltaTime(0.0f),
        showUI(true),
        showPreview(false),
        selectedKeyframe(0) {
        
        instance = this;
        
        // 전환 초기화
        currentTransition.isActive = false;
        
        // 기본 애니메이션 프리셋 초기화
        InitializePresets();
        
        // 통계 초기화
        stats.totalTransitions = 0;
        stats.totalPlayTime = 0.0f;
        stats.sessionStart = std::chrono::steady_clock::now();
    }
    
    ~CameraTransitionSystem() {
        Shutdown();
        instance = nullptr;
    }
    
    static CameraTransitionSystem* GetInstance() {
        return instance;
    }
    
    bool Initialize() {
        std::wcout << L"카메라 전환 시스템 초기화 중..." << std::endl;
        
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
        
        // 시간 초기화
        lastUpdateTime = std::chrono::high_resolution_clock::now();
        
        // 입력 스레드 시작
        inputThreadRunning = true;
        inputThread = std::thread(&CameraTransitionSystem::InputThreadFunction, this);
        
        // 설정 로드
        LoadSettings();
        
        isInitialized = true;
        std::wcout << L"카메라 전환 시스템 초기화 완료" << std::endl;
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
        
        // 원본 카메라 복원
        if (isEnabled) {
            RestoreOriginalCamera();
        }
        
        isInitialized = false;
        std::wcout << L"카메라 전환 시스템 종료" << std::endl;
    }
    
    void Update() {
        if (!isInitialized || !isEnabled) return;
        
        UpdateDeltaTime();
        
        // 키프레임 애니메이션 업데이트
        if (isPlaying && !keyframes.empty()) {
            UpdateKeyframeAnimation();
        }
        
        // 단순 전환 업데이트
        if (currentTransition.isActive) {
            UpdateSimpleTransition();
        }
        
        // WriteProcessMemory(GetCurrentProcess(), ... ) 대신 실제 게임 프로세스 핸들 사용 필요
        WriteCameraData();
        
        if (showUI) {
            DisplayUI();
        }
    }
    
    void Enable(bool enable) {
        if (!isInitialized) return;
        
        isEnabled = enable;
        
        if (enable) {
            std::wcout << L"카메라 전환 시스템 활성화" << std::endl;
        } else {
            std::wcout << L"카메라 전환 시스템 비활성화" << std::endl;
            StopAllAnimations();
            RestoreOriginalCamera();
        }
    }
    
    // 키프레임 시스템
    void AddKeyframe(float time, const CameraData& state, EaseType easeType = EaseType::Linear, const std::string& name = "") {
        CameraKeyframe keyframe(time, state, easeType, name);
        
        // 시간 순서로 정렬하여 삽입
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), keyframe,
            [](const CameraKeyframe& a, const CameraKeyframe& b) {
                return a.time < b.time;
            });
        
        keyframes.insert(it, keyframe);
        UpdateTotalDuration();
        
        std::wcout << L"키프레임 추가: " << time << L"초 (" 
                  << std::wstring(name.begin(), name.end()) << L")" << std::endl;
    }
    
    void AddCurrentPositionAsKeyframe(float time, EaseType easeType = EaseType::Linear, const std::string& name = "") {
        ReadCameraData();
        AddKeyframe(time, currentCamera, easeType, name);
    }
    
    void RemoveKeyframe(int index) {
        if (index >= 0 && index < keyframes.size()) {
            std::wcout << L"키프레임 제거: " << index << std::endl;
            keyframes.erase(keyframes.begin() + index);
            UpdateTotalDuration();
        }
    }
    
    void ClearKeyframes() {
        keyframes.clear();
        totalDuration = 0.0f;
        std::wcout << L"모든 키프레임 제거됨" << std::endl;
    }
    
    // 애니메이션 재생 제어
    void PlayAnimation() {
        if (keyframes.empty()) {
            std::wcout << L"재생할 키프레임이 없습니다." << std::endl;
            return;
        }
        
        currentTime = 0.0f;
        isPlaying = true;
        std::wcout << L"애니메이션 재생 시작 (총 " << totalDuration << L"초)" << std::endl;
    }
    
    void StopAnimation() {
        isPlaying = false;
        std::wcout << L"애니메이션 중지" << std::endl;
    }
    
    void PauseAnimation() {
        isPlaying = !isPlaying;
        std::wcout << (isPlaying ? L"애니메이션 재개" : L"애니메이션 일시정지") << std::endl;
    }
    
    void SeekTo(float time) {
        currentTime = std::max(0.0f, std::min(totalDuration, time));
        if (!keyframes.empty()) {
            ApplyKeyframeAtTime(currentTime);
        }
        std::wcout << L"시간 이동: " << currentTime << L"초" << std::endl;
    }
    
    // 단순 전환
    void TransitionTo(const CameraData& targetState, float duration, EaseType easeType = EaseType::EaseInOutQuad) {
        ReadCameraData();
        
        currentTransition.startState = currentCamera;
        currentTransition.endState = targetState;
        currentTransition.duration = duration;
        currentTransition.elapsed = 0.0f;
        currentTransition.easeType = easeType;
        currentTransition.isActive = true;
        currentTransition.onComplete = nullptr;
        
        std::wcout << L"카메라 전환 시작 (" << duration << L"초)" << std::endl;
    }
    
    void TransitionToPosition(const XMFLOAT3& position, float duration, EaseType easeType = EaseType::EaseInOutQuad) {
        ReadCameraData();
        CameraData target = currentCamera;
        target.position = position;
        TransitionTo(target, duration, easeType);
    }
    
    void TransitionToRotation(const XMFLOAT3& rotation, float duration, EaseType easeType = EaseType::EaseInOutQuad) {
        ReadCameraData();
        CameraData target = currentCamera;
        target.rotation = rotation;
        TransitionTo(target, duration, easeType);
    }
    
    // 프리셋 시스템
    void LoadPreset(int index) {
        if (index >= 0 && index < presets.size()) {
            ClearKeyframes();
            keyframes = presets[index].keyframes;
            UpdateTotalDuration();
            
            std::wcout << L"프리셋 로드: " << std::wstring(presets[index].name.begin(), presets[index].name.end()) << std::endl;
        }
    }
    
    void SaveAsPreset(const std::string& name, const std::string& description) {
        AnimationPreset preset;
        preset.name = name;
        preset.description = description;
        preset.keyframes = keyframes;
        presets.push_back(preset);
        
        std::wcout << L"프리셋 저장: " << std::wstring(name.begin(), name.end()) << std::endl;
    }
    
    // 파일 I/O
    void SaveSequenceToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        
        file << keyframes.size() << "\n";
        for (const auto& kf : keyframes) {
            file << kf.time << " "
                 << kf.cameraState.position.x << " " << kf.cameraState.position.y << " " << kf.cameraState.position.z << " "
                 << kf.cameraState.rotation.x << " " << kf.cameraState.rotation.y << " " << kf.cameraState.rotation.z << " "
                 << kf.cameraState.fov << " "
                 << static_cast<int>(kf.easeType) << " "
                 << kf.name << "\n";
        }
        
        file.close();
        std::wcout << L"시퀀스 저장: " << std::wstring(filename.begin(), filename.end()) << std::endl;
    }
    
    void LoadSequenceFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        
        ClearKeyframes();
        
        int count;
        file >> count;
        
        for (int i = 0; i < count; i++) {
            CameraKeyframe kf;
            int easeInt;
            
            file >> kf.time
                 >> kf.cameraState.position.x >> kf.cameraState.position.y >> kf.cameraState.position.z
                 >> kf.cameraState.rotation.x >> kf.cameraState.rotation.y >> kf.cameraState.rotation.z
                 >> kf.cameraState.fov
                 >> easeInt
                 >> kf.name;
            
            kf.easeType = static_cast<EaseType>(easeInt);
            keyframes.push_back(kf);
        }
        
        file.close();
        UpdateTotalDuration();
        std::wcout << L"시퀀스 로드: " << std::wstring(filename.begin(), filename.end()) 
                  << L" (" << keyframes.size() << L"개 키프레임)" << std::endl;
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
    
    void UpdateDeltaTime() {
        auto currentTimePoint = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(currentTimePoint - lastUpdateTime).count();
        lastUpdateTime = currentTimePoint;
    }
    
    void UpdateTotalDuration() {
        if (keyframes.empty()) {
            totalDuration = 0.0f;
        }
        else {
            totalDuration = keyframes.back().time;
        }
    }
    
    void UpdateKeyframeAnimation() {
        currentTime += deltaTime * playbackSpeed;
        
        if (currentTime >= totalDuration) {
            if (isLooping) {
                currentTime = 0.0f;
            } else {
                currentTime = totalDuration;
                isPlaying = false;
                std::wcout << L"애니메이션 완료" << std::endl;
                stats.totalTransitions++;
                stats.totalPlayTime += totalDuration;
            }
        }
        
        ApplyKeyframeAtTime(currentTime);
    }
    
    void ApplyKeyframeAtTime(float time) {
        if (keyframes.empty()) return;
        
        // 현재 시간에 해당하는 키프레임 찾기
        CameraKeyframe* prevKeyframe = nullptr;
        CameraKeyframe* nextKeyframe = nullptr;
        
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (keyframes[i].time <= time) {
                prevKeyframe = &keyframes[i];
            } else {
                nextKeyframe = &keyframes[i];
                break;
            }
        }
        
        if (!prevKeyframe) {
            // 시작 전
            return;
        }
        
        if (!nextKeyframe) {
            // 마지막 키프레임
            currentCamera = prevKeyframe->cameraState;
            return;
        }
        
        // 키프레임 간 보간
        float t = (time - prevKeyframe->time) / (nextKeyframe->time - prevKeyframe->time);
        t = ApplyEasing(t, nextKeyframe->easeType);
        
        currentCamera = InterpolateCameraData(prevKeyframe->cameraState, nextKeyframe->cameraState, t);
    }
    
    void UpdateSimpleTransition() {
        currentTransition.elapsed += deltaTime;
        
        if (currentTransition.elapsed >= currentTransition.duration) {
            currentCamera = currentTransition.endState;
            currentTransition.isActive = false;
            
            if (currentTransition.onComplete) {
                currentTransition.onComplete();
            }
            
            std::wcout << L"전환 완료" << std::endl;
            stats.totalTransitions++;
        }
        else {
            float t = currentTransition.elapsed / currentTransition.duration;
            t = ApplyEasing(t, currentTransition.easeType);
            
            currentCamera = InterpolateCameraData(currentTransition.startState, currentTransition.endState, t);
        }
    }
    
    CameraData InterpolateCameraData(const CameraData& a, const CameraData& b, float t) {
        CameraData result;
        
        // 위치 보간
        result.position.x = Lerp(a.position.x, b.position.x, t);
        result.position.y = Lerp(a.position.y, b.position.y, t);
        result.position.z = Lerp(a.position.z, b.position.z, t);
        
        // 회전 보간 (각도를 고려한 보간)
        result.rotation.x = LerpAngle(a.rotation.x, b.rotation.x, t);
        result.rotation.y = LerpAngle(a.rotation.y, b.rotation.y, t);
        result.rotation.z = LerpAngle(a.rotation.z, b.rotation.z, t);
        
        // FOV 보간
        result.fov = Lerp(a.fov, b.fov, t);
        
        // 나머지 필드
        result.nearPlane = Lerp(a.nearPlane, b.nearPlane, t);
        result.farPlane = Lerp(a.farPlane, b.farPlane, t);
        result.aspectRatio = Lerp(a.aspectRatio, b.aspectRatio, t);
        
        return result;
    }
    
    float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    float LerpAngle(float a, float b, float t) {
        float diff = b - a;
        while (diff > XM_PI) diff -= XM_2PI;
        while (diff < -XM_PI) diff += XM_2PI;
        return a + diff * t;
    }
    
    float ApplyEasing(float t, EaseType easeType) {
        switch (easeType) {
            case EaseType::Linear:
                return t;
                
            case EaseType::EaseInQuad:
                return t * t;
                
            case EaseType::EaseOutQuad:
                return t * (2.0f - t);
                
            case EaseType::EaseInOutQuad:
                return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
                
            case EaseType::EaseInCubic:
                return t * t * t;
                
            case EaseType::EaseOutCubic:
                return (--t) * t * t + 1.0f;
                
            case EaseType::EaseInOutCubic:
                return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
                
            case EaseType::EaseInSine:
                return 1.0f - std::cos(t * XM_PIDIV2);
                
            case EaseType::EaseOutSine:
                return std::sin(t * XM_PIDIV2);
                
            case EaseType::EaseInOutSine:
                return -(std::cos(XM_PI * t) - 1.0f) / 2.0f;
                
            case EaseType::EaseInBack:
                {
                    const float c1 = 1.70158f;
                    const float c3 = c1 + 1.0f;
                    return c3 * t * t * t - c1 * t * t;
                }
                
            case EaseType::EaseOutBack:
                {
                    const float c1 = 1.70158f;
                    const float c3 = c1 + 1.0f;
                    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
                }
                
            default:
                return t;
        }
    }
    
    void StopAllAnimations() {
        isPlaying = false;
        currentTransition.isActive = false;
        currentTime = 0.0f;
    }
    
    void InitializePresets() {
        // 기본 애니메이션 프리셋들
        
        // 원형 회전
        {
            AnimationPreset preset;
            preset.name = "Circular Rotation";
            preset.description = "카메라가 중심점 주위를 원형으로 회전";
            
            XMFLOAT3 center = {0, 0, 0};
            float radius = 10.0f;
            float height = 5.0f;
            
            for (int i = 0; i <= 8; i++) {
                float angle = i * XM_2PI / 8.0f;
                float time = i * 1.0f;
                
                CameraData state = {};
                state.position.x = center.x + radius * std::cos(angle);
                state.position.y = center.y + height;
                state.position.z = center.z + radius * std::sin(angle);
                state.rotation.y = angle + XM_PIDIV2;
                state.fov = XMConvertToRadians(90.0f);
                
                preset.keyframes.emplace_back(time, state, EaseType::EaseInOutSine);
            }
            
            presets.push_back(preset);
        }
        
        // 상승 및 하강
        {
            AnimationPreset preset;
            preset.name = "Vertical Movement";
            preset.description = "카메라가 수직으로 상승했다가 하강";
            
            CameraData baseState = originalCamera;
            
            // 시작점
            preset.keyframes.emplace_back(0.0f, baseState, EaseType::Linear);
            
            // 상승
            CameraData upState = baseState;
            upState.position.y += 20.0f;
            upState.rotation.x = -XM_PIDIV4;
            preset.keyframes.emplace_back(3.0f, upState, EaseType::EaseOutQuad);
            
            // 정점에서 잠시 대기
            preset.keyframes.emplace_back(4.0f, upState, EaseType::Linear);
            
            // 하강
            preset.keyframes.emplace_back(7.0f, baseState, EaseType::EaseInQuad);
            
            presets.push_back(preset);
        }
        
        // FOV 변화 시퀀스
        {
            AnimationPreset preset;
            preset.name = "FOV Sequence";
            preset.description = "FOV가 변화하는 시네마틱 시퀀스";
            
            CameraData state = originalCamera;
            
            // 망원 (좁은 FOV)
            state.fov = XMConvertToRadians(30.0f);
            preset.keyframes.emplace_back(0.0f, state, EaseType::Linear);
            
            // 일반 FOV로 전환
            state.fov = XMConvertToRadians(75.0f);
            preset.keyframes.emplace_back(2.0f, state, EaseType::EaseInOutCubic);
            
            // 광각 (넓은 FOV)
            state.fov = XMConvertToRadians(120.0f);
            preset.keyframes.emplace_back(4.0f, state, EaseType::EaseInOutCubic);
            
            // 원래대로
            state.fov = originalCamera.fov;
            preset.keyframes.emplace_back(6.0f, state, EaseType::EaseInOutCubic);
            
            presets.push_back(preset);
        }
    }
    
    void InputThreadFunction() {
        while (inputThreadRunning) {
            UpdateKeyStates();
            ProcessHotkeys();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
    
    void UpdateKeyStates() {
        previousKeyStates = keyStates;
        
        // 기본 제어 키
        keyStates[VK_F11] = GetAsyncKeyState(VK_F11) & 0x8000;
        keyStates[VK_F12] = GetAsyncKeyState(VK_F12) & 0x8000;
        keyStates[VK_SPACE] = GetAsyncKeyState(VK_SPACE) & 0x8000;
        keyStates[VK_RETURN] = GetAsyncKeyState(VK_RETURN) & 0x8000;
        keyStates[VK_ESCAPE] = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
        keyStates['P'] = GetAsyncKeyState('P') & 0x8000;
        keyStates['R'] = GetAsyncKeyState('R') & 0x8000;
        keyStates['L'] = GetAsyncKeyState('L') & 0x8000;
        keyStates['S'] = GetAsyncKeyState('S') & 0x8000;
        keyStates['H'] = GetAsyncKeyState('H') & 0x8000;
        keyStates['K'] = GetAsyncKeyState('K') & 0x8000;
        keyStates['C'] = GetAsyncKeyState('C') & 0x8000;
        keyStates[VK_DELETE] = GetAsyncKeyState(VK_DELETE) & 0x8000;
        keyStates[VK_LEFT] = GetAsyncKeyState(VK_LEFT) & 0x8000;
        keyStates[VK_RIGHT] = GetAsyncKeyState(VK_RIGHT) & 0x8000;
        keyStates[VK_UP] = GetAsyncKeyState(VK_UP) & 0x8000;
        keyStates[VK_DOWN] = GetAsyncKeyState(VK_DOWN) & 0x8000;
        
        // 숫자 키 (프리셋 선택)
        for (int i = '1'; i <= '9'; i++) {
            keyStates[i] = GetAsyncKeyState(i) & 0x8000;
        }
    }
    
    bool IsKeyPressed(int key) {
        return keyStates[key] && !previousKeyStates[key];
    }
    
    void ProcessHotkeys() {
        // F11: 전환 시스템 토글
        if (IsKeyPressed(VK_F11)) {
            Enable(!isEnabled);
        }
        
        // F12: UI 토글
        if (IsKeyPressed(VK_F12)) {
            showUI = !showUI;
        }
        
        if (!isEnabled) return;
        
        // Space: 재생/일시정지
        if (IsKeyPressed(VK_SPACE)) {
            if (!keyframes.empty()) {
                PauseAnimation();
            }
        }
        
        // Enter: 재생
        if (IsKeyPressed(VK_RETURN)) {
            PlayAnimation();
        }
        
        // Escape: 정지
        if (IsKeyPressed(VK_ESCAPE)) {
            StopAnimation();
        }
        
        // K: 현재 위치를 키프레임으로 추가
        if (IsKeyPressed('K')) {
            float time = keyframes.empty() ? 0.0f : totalDuration + 1.0f;
            AddCurrentPositionAsKeyframe(time, EaseType::EaseInOutQuad, "Manual");
        }
        
        // C: 키프레임 모두 지우기
        if (IsKeyPressed('C')) {
            ClearKeyframes();
        }
        
        // Delete: 선택된 키프레임 삭제
        if (IsKeyPressed(VK_DELETE)) {
            selectedKeyframe = std::min(selectedKeyframe, static_cast<int>(keyframes.size()) - 1);
            if (selectedKeyframe >= 0) {
                RemoveKeyframe(selectedKeyframe);
            }
        }
        
        // L: 루프 토글
        if (IsKeyPressed('L')) {
            isLooping = !isLooping;
            std::wcout << (isLooping ? L"루프 활성화" : L"루프 비활성화") << std::endl;
        }
        
        // 화살표 키: 시간 이동 및 키프레임 선택
        if (IsKeyPressed(VK_LEFT)) {
            SeekTo(currentTime - 1.0f);
        }
        if (IsKeyPressed(VK_RIGHT)) {
            SeekTo(currentTime + 1.0f);
        }
        if (IsKeyPressed(VK_UP)) {
            selectedKeyframe = std::max(0, selectedKeyframe - 1);
        }
        if (IsKeyPressed(VK_DOWN)) {
            selectedKeyframe = std::min(static_cast<int>(keyframes.size()) - 1, selectedKeyframe + 1);
        }
        
        // 숫자 키: 프리셋 로드
        for (int i = '1'; i <= '9'; i++) {
            if (IsKeyPressed(i)) {
                int index = i - '1';
                if (index < presets.size()) {
                    LoadPreset(index);
                }
            }
        }
    }
    
    void DisplayUI() {
        static auto lastDisplayTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        
        if (now - lastDisplayTime > std::chrono::milliseconds(100)) {
            // system("cls"); // Windows 전용, 깜빡임 발생 가능
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            COORD coord = {0, 0};
            SetConsoleCursorPosition(hConsole, coord);
            
            std::wcout << L"╔════════════════════════════════════════════════════════╗" << std::endl;
            std::wcout << L"║                Camera Transition System                ║" << std::endl;
            std::wcout << L"╠════════════════════════════════════════════════════════╣" << std::endl;
            
            // 재생 상태
            std::wcout << L"║ Status: " << (isPlaying ? L"PLAYING" : L"STOPPED") << L"    ";
            if (!keyframes.empty()) {
                std::wcout << L"Time: " << std::fixed << std::setprecision(1) << currentTime 
                          << L"/" << totalDuration << L"s";
            }
            std::wcout << L"       ║" << std::endl;
            
            // 키프레임 정보
            std::wcout << L"║ Keyframes: " << keyframes.size() << L"    ";
            if (!keyframes.empty()) {
                std::wcout << L"Selected: " << (selectedKeyframe + 1);
            }
            std::wcout << L"                           ║" << std::endl;
            
            // 전환 상태
            if (currentTransition.isActive) {
                float progress = currentTransition.elapsed / currentTransition.duration * 100.0f;
                std::wcout << L"║ Transition: " << std::fixed << std::setprecision(1) 
                          << progress << L"%                                  ║" << std::endl;
            }
            
            std::wcout << L"╠════════════════════════════════════════════════════════╣" << std::endl;
            std::wcout << L"║ Controls:                                              ║" << std::endl;
            std::wcout << L"║ Space: Play/Pause  Enter: Play  Esc: Stop             ║" << std::endl;
            std::wcout << L"║ K: Add Keyframe    C: Clear      Del: Remove Selected  ║" << std::endl;
            std::wcout << L"║ L: Toggle Loop     H: Save/Load  1-9: Load Preset     ║" << std::endl;
            std::wcout << L"║ ←→: Seek Time     ↑↓: Select Keyframe               ║" << std::endl;
            std::wcout << L"╚════════════════════════════════════════════════════════╝" << std::endl;
            
            // 진행 바
            if (!keyframes.empty()) {
                int barWidth = 50;
                float progress = currentTime / totalDuration;
                int filledWidth = static_cast<int>(progress * barWidth);
                
                std::wcout << L"Progress: [";
                for (int i = 0; i < barWidth; i++) {
                    if (i < filledWidth) {
                        std::wcout << L"█";
                    } else {
                        std::wcout << L"░";
                    }
                }
                std::wcout << L"]" << std::endl;
            }
            
            lastDisplayTime = now;
        }
    }
    
    void LoadSettings() {
        std::ifstream file("camera_transition_settings.txt");
        if (file.is_open()) {
            file >> playbackSpeed >> isLooping >> showUI;
            file.close();
        }
    }
    
    void SaveSettings() {
        std::ofstream file("camera_transition_settings.txt");
        if (file.is_open()) {
            file << playbackSpeed << " " << isLooping << " " << showUI;
            file.close();
        }
    }
    
    void PrintControls() {
        std::wcout << L"\n=== 카메라 전환 시스템 조작법 ===" << std::endl;
        std::wcout << L"F11: 전환 시스템 토글" << std::endl;
        std::wcout << L"F12: UI 토글" << std::endl;
        std::wcout << L"\n[애니메이션 제어]" << std::endl;
        std::wcout << L"Space: 재생/일시정지" << std::endl;
        std::wcout << L"Enter: 재생 시작" << std::endl;
        std::wcout << L"Esc: 정지" << std::endl;
        std::wcout << L"←→: 시간 이동" << std::endl;
        std::wcout << L"L: 루프 토글" << std::endl;
        std::wcout << L"\n[키프레임 편집]" << std::endl;
        std::wcout << L"K: 현재 위치를 키프레임으로 추가" << std::endl;
        std::wcout << L"C: 모든 키프레임 지우기" << std::endl;
        std::wcout << L"Del: 선택된 키프레임 삭제" << std::endl;
        std::wcout << L"↑↓: 키프레임 선택" << std::endl;
        std::wcout << L"\n[프리셋]" << std::endl;
        std::wcout << L"1-9: 프리셋 로드" << std::endl;
        std::wcout << L"============================\n" << std::endl;
    }
};

// 정적 멤버 정의
CameraTransitionSystem* CameraTransitionSystem::instance = nullptr;

// DLL 진입점
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    static CameraTransitionSystem* system = nullptr;
    
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            AllocConsole();
            freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
            freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
            freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
            
            std::wcout << L"카메라 전환 시스템 DLL 로드됨" << std::endl;
            
            system = new CameraTransitionSystem();
            if (!system->Initialize()) {
                delete system;
                system = nullptr;
                std::wcout << L"카메라 전환 시스템 초기화 실패" << std::endl;
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
extern "C" __declspec(dllexport) void UpdateCameraTransition() {
    CameraTransitionSystem* system = CameraTransitionSystem::GetInstance();
    if (system) {
        system->Update();
    }
}

extern "C" __declspec(dllexport) void EnableCameraTransition(bool enable) {
    CameraTransitionSystem* system = CameraTransitionSystem::GetInstance();
    if (system) {
        system->Enable(enable);
    }
}

extern "C" __declspec(dllexport) void PlayCameraAnimation() {
    CameraTransitionSystem* system = CameraTransitionSystem::GetInstance();
    if (system) {
        system->PlayAnimation();
    }
}

extern "C" __declspec(dllexport) void StopCameraAnimation() {
    CameraTransitionSystem* system = CameraTransitionSystem::GetInstance();
    if (system) {
        system->StopAnimation();
    }
}

// 독립 실행형 테스트
#ifdef STANDALONE_TEST
int main() {
    std::wcout << L"=== 카메라 전환 시스템 테스트 ===" << std::endl;
    
    CameraTransitionSystem system;
    
    if (!system.Initialize()) {
        std::wcout << L"초기화 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    system.Enable(true);
    
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
