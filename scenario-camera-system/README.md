# 🟡 카메라 시스템 수정

**난이도**: 중급 | **학습 시간**: 2-3주 | **접근법**: 3D 수학 + 함수 후킹

게임의 카메라 시스템을 수정하여 FOV, 시점, 움직임 등을 개선하는 모딩 기법을 학습합니다.

## 📖 학습 목표

이 과정를 완료하면 다음을 할 수 있게 됩니다:

- [ ] 3D 카메라 수학 원리 이해하기 (투영 행렬, 뷰 행렬)
- [ ] FOV(시야각) 값을 메모리에서 찾고 수정하기
- [ ] 카메라 위치/회전 데이터 조작하기
- [ ] 프리 카메라 모드 구현하기
- [ ] 카메라 애니메이션 및 부드러운 이동 구현하기

## 🎯 최종 결과물

완성된 모드의 기능:
- **동적 FOV 조정** (실시간 슬라이더)
- **프리 카메라 모드** (노클립 시점 이동)
- **카메라 위치 저장/로드** (즐겨찾기 시점)
- **부드러운 카메라 전환** (Ease In/Out 애니메이션)
- **3인칭 ↔ 1인칭 전환** (게임별 맞춤)

## 📐 3D 카메라 수학 기초

### 1. 카메라 변환 파이프라인
```
3D 좌표 변환 과정:
World Space → View Space → Projection Space → Screen Space
     ↑             ↑              ↑              ↑
   모델 배치    카메라 변환     원근/직교 투영   화면 좌표
```

### 2. 핵심 행렬들
```cpp
// 뷰 행렬 (카메라 위치/방향)
Matrix4x4 viewMatrix = LookAt(cameraPos, targetPos, upVector);

// 투영 행렬 (FOV, 종횡비, Near/Far 평면)
Matrix4x4 projMatrix = Perspective(fovY, aspectRatio, nearPlane, farPlane);

// MVP 행렬 (Model-View-Projection)
Matrix4x4 mvpMatrix = projMatrix * viewMatrix * modelMatrix;
```

### 3. FOV와 시야각 관계
```cpp
// FOV 계산 공식
float fovRadians = fovDegrees * (M_PI / 180.0f);
float fovHorizontal = 2.0f * atan(tan(fovVertical / 2.0f) * aspectRatio);

// 일반적인 FOV 값들
enum CommonFOV {
    FOV_NARROW = 60,     // 망원경 효과
    FOV_NORMAL = 90,     // 기본값
    FOV_WIDE = 120,      // 광각 효과
    FOV_FISHEYE = 170    // 어안렌즈 효과
};
```

## 🔍 카메라 데이터 찾기

### Cheat Engine을 이용한 FOV 스캔

```bash
1단계: 기본 FOV 값 찾기
- Value Type: Float
- Scan Type: Exact Value  
- Value: 90.0 (일반적인 기본 FOV)
- First Scan 실행

2단계: FOV 변경하여 확인
- 게임 설정에서 FOV 변경 (가능한 경우)
- 또는 콘솔 명령어 사용 (fov 90 → fov 120)
- Changed value로 재스캔

3단계: 주소 검증
- 찾은 주소의 값을 직접 수정
- 게임 화면에서 시야각 변화 확인
- 부작용 없는지 테스트
```

### 일반적인 카메라 구조체 패턴
```cpp
// 대부분 게임의 카메라 데이터 구조
struct CameraData {
    Vector3 position;        // 카메라 위치 (x, y, z)
    Vector3 rotation;        // 카메라 회전 (pitch, yaw, roll)
    float fov;              // 시야각 (degrees 또는 radians)
    float nearPlane;        // 근접 평면 거리
    float farPlane;         // 원거리 평면 거리
    float aspectRatio;      // 화면 비율 (width/height)
    
    // 선택적 요소들
    Vector3 targetPosition; // 3인칭 카메라 타겟
    float distance;         // 타겟으로부터의 거리
    float sensitivity;      // 마우스 감도
    bool isFirstPerson;     // 1인칭/3인칭 모드
};
```

## 💻 카메라 모드 구현

### 기본 카메라 컨트롤러

```cpp
// CameraController.h
#pragma once
#include <Windows.h>
#include <DirectXMath.h>
#include <vector>
#include <map>

using namespace DirectX;

struct CameraState {
    XMFLOAT3 position;
    XMFLOAT3 rotation;
    float fov;
    float speed;
    bool isFreeCam;
};

struct CameraBookmark {
    std::string name;
    CameraState state;
};

class CameraController {
private:
    static uintptr_t cameraAddress;
    static CameraState currentState;
    static CameraState originalState;
    static std::vector<CameraBookmark> bookmarks;
    static bool isInitialized;
    static bool keysPressed[256];

public:
    static bool Initialize();
    static void Update();
    static void Shutdown();
    
    // 기본 기능
    static bool FindCameraData();
    static void SetFOV(float fov);
    static void SetPosition(const XMFLOAT3& pos);
    static void SetRotation(const XMFLOAT3& rot);
    
    // 프리 카메라
    static void EnableFreeCam(bool enable);
    static void ProcessFreeCamInput();
    static void MoveCamera(const XMFLOAT3& direction, float deltaTime);
    
    // 북마크 시스템
    static void SaveBookmark(const std::string& name);
    static void LoadBookmark(const std::string& name);
    static std::vector<CameraBookmark> GetBookmarks() { return bookmarks; }
    
    // 유틸리티
    static CameraState GetCurrentState() { return currentState; }
    static void RestoreOriginalState();
    static bool IsFreeCamEnabled() { return currentState.isFreeCam; }

private:
    static bool ReadCameraData();
    static bool WriteCameraData();
    static void ProcessKeyInput();
    static XMFLOAT3 GetMovementInput();
    static XMFLOAT2 GetRotationInput();
};

// CameraController.cpp
#include "CameraController.h"
#include "MemoryUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>

// 전역 변수 초기화
uintptr_t CameraController::cameraAddress = 0;
CameraState CameraController::currentState = {};
CameraState CameraController::originalState = {};
std::vector<CameraBookmark> CameraController::bookmarks;
bool CameraController::isInitialized = false;
bool CameraController::keysPressed[256] = {};

bool CameraController::Initialize() {
    if (!FindCameraData()) {
        std::cout << "카메라 데이터를 찾을 수 없습니다." << std::endl;
        return false;
    }
    
    // 원본 상태 백업
    ReadCameraData();
    originalState = currentState;
    
    // 북마크 로드
    LoadBookmarksFromFile();
    
    isInitialized = true;
    std::cout << "카메라 컨트롤러 초기화 완료" << std::endl;
    return true;
}

bool CameraController::FindCameraData() {
    // 90.0f (기본 FOV) 값 검색
    std::vector<uintptr_t> fovAddresses = MemoryUtils::ScanForFloat(90.0f);
    
    for (auto addr : fovAddresses) {
        // FOV 주변에 카메라 데이터가 있는지 확인
        if (ValidateCameraStructure(addr)) {
            // FOV 위치를 기준으로 전체 구조체 주소 계산
            cameraAddress = addr - offsetof(CameraData, fov);
            std::cout << "카메라 주소 발견: 0x" << std::hex << cameraAddress << std::endl;
            return true;
        }
    }
    
    return false;
}

bool CameraController::ValidateCameraStructure(uintptr_t fovAddr) {
    // FOV 주변 메모리가 유효한 카메라 데이터인지 검증
    
    // 위치 데이터 확인 (일반적으로 FOV 앞에 위치)
    XMFLOAT3 pos;
    if (!MemoryUtils::ReadMemory(fovAddr - 24, &pos, sizeof(XMFLOAT3))) {
        return false;
    }
    
    // 합리적인 좌표 범위인지 확인
    if (abs(pos.x) > 10000.0f || abs(pos.y) > 10000.0f || abs(pos.z) > 10000.0f) {
        return false;
    }
    
    // FOV 값이 합리적인 범위인지 확인
    float fov;
    if (!MemoryUtils::ReadMemory(fovAddr, &fov, sizeof(float))) {
        return false;
    }
    
    if (fov < 10.0f || fov > 180.0f) {
        return false;
    }
    
    return true;
}

void CameraController::Update() {
    if (!isInitialized) return;
    
    ProcessKeyInput();
    
    if (currentState.isFreeCam) {
        ProcessFreeCamInput();
    }
    
    WriteCameraData();
}

void CameraController::ProcessKeyInput() {
    // 키 상태 업데이트
    for (int i = 0; i < 256; i++) {
        keysPressed[i] = GetAsyncKeyState(i) & 0x8000;
    }
    
    // 단축키 처리
    if (keysPressed[VK_F1] && !keysPressed[VK_F1]) { // F1 키 눌림
        EnableFreeCam(!currentState.isFreeCam);
    }
    
    if (keysPressed[VK_F2]) { // F2: FOV 증가
        SetFOV(currentState.fov + 1.0f);
    }
    
    if (keysPressed[VK_F3]) { // F3: FOV 감소
        SetFOV(currentState.fov - 1.0f);
    }
    
    if (keysPressed[VK_F4]) { // F4: 원본 복원
        RestoreOriginalState();
    }
    
    // 북마크 저장/로드 (Ctrl + 숫자)
    if (keysPressed[VK_CONTROL]) {
        for (int i = '0'; i <= '9'; i++) {
            if (keysPressed[i]) {
                if (keysPressed[VK_SHIFT]) {
                    // Ctrl+Shift+숫자: 저장
                    SaveBookmark("Bookmark " + std::to_string(i - '0'));
                } else {
                    // Ctrl+숫자: 로드
                    LoadBookmark("Bookmark " + std::to_string(i - '0'));
                }
            }
        }
    }
}

void CameraController::ProcessFreeCamInput() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    // 이동 입력 처리
    XMFLOAT3 movement = GetMovementInput();
    if (movement.x != 0 || movement.y != 0 || movement.z != 0) {
        MoveCamera(movement, deltaTime);
    }
    
    // 회전 입력 처리 (마우스)
    XMFLOAT2 rotation = GetRotationInput();
    if (rotation.x != 0 || rotation.y != 0) {
        currentState.rotation.x += rotation.y * 0.1f; // Pitch
        currentState.rotation.y += rotation.x * 0.1f; // Yaw
        
        // Pitch 제한 (-90도 ~ +90도)
        currentState.rotation.x = max(-90.0f, min(90.0f, currentState.rotation.x));
    }
}

XMFLOAT3 CameraController::GetMovementInput() {
    XMFLOAT3 movement = {0, 0, 0};
    float speed = currentState.speed;
    
    // 속도 조절
    if (keysPressed[VK_SHIFT]) speed *= 3.0f;  // 빠르게
    if (keysPressed[VK_CONTROL]) speed *= 0.3f; // 느리게
    
    // WASD 이동
    if (keysPressed['W']) movement.z += speed;
    if (keysPressed['S']) movement.z -= speed;
    if (keysPressed['A']) movement.x -= speed;
    if (keysPressed['D']) movement.x += speed;
    
    // 상하 이동
    if (keysPressed[VK_SPACE]) movement.y += speed;
    if (keysPressed['C']) movement.y -= speed;
    
    return movement;
}

XMFLOAT2 CameraController::GetRotationInput() {
    static POINT lastMousePos = {};
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);
    
    XMFLOAT2 rotation = {
        static_cast<float>(currentMousePos.x - lastMousePos.x),
        static_cast<float>(currentMousePos.y - lastMousePos.y)
    };
    
    lastMousePos = currentMousePos;
    
    // 마우스 감도 적용
    rotation.x *= 0.1f;
    rotation.y *= 0.1f;
    
    return rotation;
}

void CameraController::MoveCamera(const XMFLOAT3& direction, float deltaTime) {
    // 카메라 방향 벡터 계산
    float yawRad = XMConvertToRadians(currentState.rotation.y);
    float pitchRad = XMConvertToRadians(currentState.rotation.x);
    
    XMFLOAT3 forward = {
        sin(yawRad) * cos(pitchRad),
        -sin(pitchRad),
        cos(yawRad) * cos(pitchRad)
    };
    
    XMFLOAT3 right = {
        cos(yawRad),
        0,
        -sin(yawRad)
    };
    
    XMFLOAT3 up = {0, 1, 0};
    
    // 최종 이동 벡터 계산
    XMFLOAT3 moveVector = {
        direction.x * right.x + direction.z * forward.x,
        direction.y * up.y + direction.z * forward.y,
        direction.x * right.z + direction.z * forward.z
    };
    
    // 위치 업데이트
    currentState.position.x += moveVector.x * deltaTime;
    currentState.position.y += moveVector.y * deltaTime;
    currentState.position.z += moveVector.z * deltaTime;
}

void CameraController::SetFOV(float fov) {
    // FOV 범위 제한
    fov = max(10.0f, min(179.0f, fov));
    currentState.fov = fov;
    
    std::cout << "FOV 설정: " << fov << "도" << std::endl;
}

void CameraController::EnableFreeCam(bool enable) {
    currentState.isFreeCam = enable;
    
    if (enable) {
        std::cout << "프리 카메라 모드 활성화" << std::endl;
        std::cout << "조작법:" << std::endl;
        std::cout << "- WASD: 이동" << std::endl;
        std::cout << "- Space/C: 위/아래" << std::endl;
        std::cout << "- Shift: 빠르게" << std::endl;
        std::cout << "- Ctrl: 느리게" << std::endl;
        std::cout << "- 마우스: 회전" << std::endl;
    } else {
        std::cout << "프리 카메라 모드 비활성화" << std::endl;
    }
}

void CameraController::SaveBookmark(const std::string& name) {
    CameraBookmark bookmark;
    bookmark.name = name;
    bookmark.state = currentState;
    
    // 기존 북마크 찾기
    auto it = std::find_if(bookmarks.begin(), bookmarks.end(),
        [&name](const CameraBookmark& b) { return b.name == name; });
    
    if (it != bookmarks.end()) {
        *it = bookmark; // 덮어쓰기
    } else {
        bookmarks.push_back(bookmark); // 새로 추가
    }
    
    SaveBookmarksToFile();
    std::cout << "북마크 저장: " << name << std::endl;
}

void CameraController::LoadBookmark(const std::string& name) {
    auto it = std::find_if(bookmarks.begin(), bookmarks.end(),
        [&name](const CameraBookmark& b) { return b.name == name; });
    
    if (it != bookmarks.end()) {
        currentState = it->state;
        std::cout << "북마크 로드: " << name << std::endl;
    } else {
        std::cout << "북마크를 찾을 수 없음: " << name << std::endl;
    }
}

bool CameraController::ReadCameraData() {
    if (cameraAddress == 0) return false;
    
    CameraData data;
    if (!MemoryUtils::ReadMemory(cameraAddress, &data, sizeof(CameraData))) {
        return false;
    }
    
    currentState.position = data.position;
    currentState.rotation = data.rotation;
    currentState.fov = data.fov;
    
    return true;
}

bool CameraController::WriteCameraData() {
    if (cameraAddress == 0) return false;
    
    CameraData data;
    data.position = currentState.position;
    data.rotation = currentState.rotation;
    data.fov = currentState.fov;
    // 다른 필드들은 원본 유지
    
    return MemoryUtils::WriteMemory(cameraAddress, &data, sizeof(CameraData));
}
```

## 🎮 고급 카메라 기능

### 1. 부드러운 카메라 전환 (Smooth Transitions)

```cpp
class CameraTweener {
private:
    struct TweenData {
        CameraState startState;
        CameraState endState;
        float duration;
        float elapsed;
        EaseType easeType;
        bool isActive;
    };
    
    static TweenData currentTween;

public:
    enum EaseType {
        LINEAR,
        EASE_IN_OUT,
        EASE_IN_CUBIC,
        EASE_OUT_CUBIC
    };
    
    static void StartTransition(const CameraState& target, float duration, EaseType easeType) {
        currentTween.startState = CameraController::GetCurrentState();
        currentTween.endState = target;
        currentTween.duration = duration;
        currentTween.elapsed = 0.0f;
        currentTween.easeType = easeType;
        currentTween.isActive = true;
    }
    
    static void Update(float deltaTime) {
        if (!currentTween.isActive) return;
        
        currentTween.elapsed += deltaTime;
        float t = currentTween.elapsed / currentTween.duration;
        
        if (t >= 1.0f) {
            t = 1.0f;
            currentTween.isActive = false;
        }
        
        // Easing 함수 적용
        float easedT = ApplyEasing(t, currentTween.easeType);
        
        // 보간된 상태 계산
        CameraState interpolated = InterpolateStates(
            currentTween.startState, 
            currentTween.endState, 
            easedT
        );
        
        // 카메라에 적용
        CameraController::SetPosition(interpolated.position);
        CameraController::SetRotation(interpolated.rotation);
        CameraController::SetFOV(interpolated.fov);
    }
    
private:
    static float ApplyEasing(float t, EaseType type) {
        switch (type) {
            case LINEAR:
                return t;
            case EASE_IN_OUT:
                return t * t * (3.0f - 2.0f * t);
            case EASE_IN_CUBIC:
                return t * t * t;
            case EASE_OUT_CUBIC:
                return 1.0f - pow(1.0f - t, 3.0f);
            default:
                return t;
        }
    }
    
    static CameraState InterpolateStates(const CameraState& a, const CameraState& b, float t) {
        CameraState result;
        result.position = Lerp(a.position, b.position, t);
        result.rotation = LerpAngles(a.rotation, b.rotation, t);
        result.fov = Lerp(a.fov, b.fov, t);
        return result;
    }
};
```

### 2. 카메라 트래킹 시스템

```cpp
class CameraTracker {
private:
    struct TrackTarget {
        uintptr_t address;      // 추적할 오브젝트 주소
        XMFLOAT3 offset;        // 카메라 오프셋
        float distance;         // 거리
        float height;           // 높이
        bool isActive;
    };
    
    static TrackTarget currentTarget;

public:
    static void SetTrackTarget(uintptr_t objectAddress, const XMFLOAT3& offset) {
        currentTarget.address = objectAddress;
        currentTarget.offset = offset;
        currentTarget.distance = 5.0f;
        currentTarget.height = 2.0f;
        currentTarget.isActive = true;
        
        std::cout << "오브젝트 추적 시작: 0x" << std::hex << objectAddress << std::endl;
    }
    
    static void Update() {
        if (!currentTarget.isActive) return;
        
        // 타겟 오브젝트 위치 읽기
        XMFLOAT3 targetPos;
        if (!MemoryUtils::ReadMemory(currentTarget.address, &targetPos, sizeof(XMFLOAT3))) {
            std::cout << "타겟 오브젝트를 읽을 수 없음" << std::endl;
            currentTarget.isActive = false;
            return;
        }
        
        // 카메라 위치 계산
        XMFLOAT3 cameraPos = {
            targetPos.x + currentTarget.offset.x,
            targetPos.y + currentTarget.height,
            targetPos.z + currentTarget.offset.z - currentTarget.distance
        };
        
        // 카메라가 타겟을 바라보도록 회전 계산
        XMFLOAT3 direction = {
            targetPos.x - cameraPos.x,
            targetPos.y - cameraPos.y,
            targetPos.z - cameraPos.z
        };
        
        // 방향 벡터를 오일러 각도로 변환
        float yaw = atan2(direction.x, direction.z) * 180.0f / M_PI;
        float pitch = -atan2(direction.y, sqrt(direction.x * direction.x + direction.z * direction.z)) * 180.0f / M_PI;
        
        XMFLOAT3 cameraRot = {pitch, yaw, 0.0f};
        
        // 카메라 위치 및 회전 적용
        CameraController::SetPosition(cameraPos);
        CameraController::SetRotation(cameraRot);
    }
    
    static void StopTracking() {
        currentTarget.isActive = false;
        std::cout << "오브젝트 추적 중지" << std::endl;
    }
};
```

### 3. 카메라 애니메이션 시퀀스

```cpp
class CameraSequence {
private:
    struct Keyframe {
        float time;
        CameraState state;
        EaseType easeType;
    };
    
    static std::vector<Keyframe> keyframes;
    static float currentTime;
    static bool isPlaying;

public:
    static void AddKeyframe(float time, const CameraState& state, EaseType easeType = LINEAR) {
        Keyframe kf;
        kf.time = time;
        kf.state = state;
        kf.easeType = easeType;
        
        // 시간 순서로 정렬 삽입
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), kf,
            [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
        
        keyframes.insert(it, kf);
        
        std::cout << "키프레임 추가: " << time << "초" << std::endl;
    }
    
    static void Play() {
        if (keyframes.empty()) {
            std::cout << "재생할 키프레임이 없습니다." << std::endl;
            return;
        }
        
        currentTime = 0.0f;
        isPlaying = true;
        std::cout << "카메라 시퀀스 재생 시작" << std::endl;
    }
    
    static void Update(float deltaTime) {
        if (!isPlaying || keyframes.empty()) return;
        
        currentTime += deltaTime;
        
        // 현재 시간에 해당하는 키프레임 찾기
        Keyframe* prevFrame = nullptr;
        Keyframe* nextFrame = nullptr;
        
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (keyframes[i].time <= currentTime) {
                prevFrame = &keyframes[i];
            } else {
                nextFrame = &keyframes[i];
                break;
            }
        }
        
        if (!prevFrame) {
            // 시작 전
            return;
        }
        
        if (!nextFrame) {
            // 시퀀스 종료
            CameraController::SetPosition(prevFrame->state.position);
            CameraController::SetRotation(prevFrame->state.rotation);
            CameraController::SetFOV(prevFrame->state.fov);
            isPlaying = false;
            std::cout << "카메라 시퀀스 재생 완료" << std::endl;
            return;
        }
        
        // 키프레임 간 보간
        float t = (currentTime - prevFrame->time) / (nextFrame->time - prevFrame->time);
        t = CameraTweener::ApplyEasing(t, nextFrame->easeType);
        
        CameraState interpolated = CameraTweener::InterpolateStates(
            prevFrame->state, nextFrame->state, t);
        
        CameraController::SetPosition(interpolated.position);
        CameraController::SetRotation(interpolated.rotation);
        CameraController::SetFOV(interpolated.fov);
    }
    
    static void Stop() {
        isPlaying = false;
        std::cout << "카메라 시퀀스 중지" << std::endl;
    }
    
    static void Clear() {
        keyframes.clear();
        std::cout << "모든 키프레임 삭제" << std::endl;
    }
    
    static void SaveToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        
        for (const auto& kf : keyframes) {
            file << kf.time << " "
                 << kf.state.position.x << " " << kf.state.position.y << " " << kf.state.position.z << " "
                 << kf.state.rotation.x << " " << kf.state.rotation.y << " " << kf.state.rotation.z << " "
                 << kf.state.fov << " "
                 << static_cast<int>(kf.easeType) << std::endl;
        }
        
        file.close();
        std::cout << "시퀀스 저장: " << filename << std::endl;
    }
    
    static void LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        
        keyframes.clear();
        
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            Keyframe kf;
            int easeInt;
            
            ss >> kf.time
               >> kf.state.position.x >> kf.state.position.y >> kf.state.position.z
               >> kf.state.rotation.x >> kf.state.rotation.y >> kf.state.rotation.z
               >> kf.state.fov
               >> easeInt;
            
            kf.easeType = static_cast<EaseType>(easeInt);
            keyframes.push_back(kf);
        }
        
        file.close();
        std::cout << "시퀀스 로드: " << filename << " (" << keyframes.size() << "개 키프레임)" << std::endl;
    }
};
```

## 🎯 실습 과제

### 과제 1: 기본 카메라 조작 (초급)
- [ ] **FOV 슬라이더**: 30-170도 범위 조정
- [ ] **위치 미세 조정**: X/Y/Z 축별 0.1 단위 이동
- [ ] **회전 스냅**: 90도 단위 회전 기능
- [ ] **시점 리셋**: 원래 위치로 즉시 복귀

### 과제 2: 프리 카메라 시스템 (중급)
- [ ] **WASD 이동**: 방향키 기반 자유 이동
- [ ] **마우스 룩**: 마우스로 시점 회전
- [ ] **속도 조절**: Shift(빠름), Ctrl(느림) 지원
- [ ] **충돌 감지**: 벽 관통 방지 기능

### 과제 3: 고급 카메라 기능 (고급)
- [ ] **시네마틱 모드**: 부드러운 카메라 이동
- [ ] **타겟 추적**: 특정 오브젝트 자동 추적
- [ ] **키프레임 애니메이션**: 복잡한 카메라 시퀀스
- [ ] **VR 카메라**: 스테레오 렌더링 지원

## 🔧 게임별 특수 고려사항

### 1. FromSoftware 게임 (EldenRing, Dark Souls)
```cpp
// FromSoft 카메라 특성
struct FromSoftCamera {
    Vector3 playerPosition;     // 플레이어 위치
    Vector3 cameraOffset;       // 카메라 오프셋  
    float distance;             // 거리
    float pitch;                // 상하 각도
    float yaw;                  // 좌우 각도
    float fov;                  // 시야각
    bool isLocked;              // 타겟 락온 상태
};

// 특별한 처리가 필요한 부분
void HandleFromSoftCamera() {
    // 1. 플레이어-카메라 상대적 위치 시스템
    // 2. 벽 충돌 감지 및 카메라 당겨오기
    // 3. 타겟 락온 시 카메라 제약
    // 4. 말/보스전 등 특수 상황 처리
}
```

### 2. FPS 게임
```cpp
// FPS 카메라 특성
struct FPSCamera {
    Vector3 headPosition;       // 머리 위치 (총구 기준점)
    Vector2 mouseInput;         // 마우스 입력
    float sensitivity;          // 마우스 감도
    float fov;                  // 시야각 (줌 스코프 고려)
    float recoilOffset;         // 반동 오프셋
    bool isAiming;              // 조준 모드
};
```

### 3. 레이싱 게임
```cpp
// 레이싱 카메라 특성
struct RacingCamera {
    Vector3 carPosition;        // 자동차 위치
    Vector3 carVelocity;        // 속도 벡터
    float followDistance;       // 추적 거리
    float heightOffset;         // 높이 오프셋
    float speedBasedFOV;        // 속도 기반 FOV
    bool isRearView;            // 후방 시점
};
```

## 📊 성능 최적화

### 1. 카메라 업데이트 빈도 제한
```cpp
class CameraOptimizer {
private:
    static float updateInterval;
    static float lastUpdateTime;

public:
    static void SetUpdateRate(float fps) {
        updateInterval = 1.0f / fps;
    }
    
    static bool ShouldUpdate() {
        float currentTime = GetTime();
        if (currentTime - lastUpdateTime >= updateInterval) {
            lastUpdateTime = currentTime;
            return true;
        }
        return false;
    }
};
```

### 2. 메모리 접근 최소화
```cpp
class CacheSystem {
private:
    static CameraState cachedState;
    static bool isDirty;

public:
    static void MarkDirty() { isDirty = true; }
    
    static const CameraState& GetCachedState() {
        if (isDirty) {
            ReadCameraFromMemory(cachedState);
            isDirty = false;
        }
        return cachedState;
    }
};
```

## 💡 트러블슈팅

### Q: FOV 변경 후 화면이 왜곡돼요
```
A: FOV 범위 확인:
- 너무 낮은 값 (<30도): 망원경 효과
- 너무 높은 값 (>170도): 어안렌즈 왜곡
- 권장 범위: 60-120도
- 종횡비 고려: FOV = 2 * atan(tan(vFOV/2) * aspectRatio)
```

### Q: 프리 카메라에서 움직임이 부자연스러워요
```
A: 움직임 보정 방법:
1. deltaTime 기반 이동으로 변경
2. 가속/감속 애니메이션 추가
3. 마우스 감도 조정
4. 키 입력 스무딩 적용
```

### Q: 특정 지역에서 카메라가 튕겨나가요
```
A: 충돌 시스템 간섭:
1. 게임 내장 충돌 감지 비활성화 필요
2. Collision 관련 메모리 주소 찾기
3. 카메라 모드별로 충돌 처리 다르게 적용
4. 안전 영역 설정으로 크래시 방지
```

## 🔗 관련 자료

- [3D Math Primer](https://gamemath.com/) - 3D 수학 기초
- [Real-Time Cameras](https://docs.unity3d.com/Manual/CamerasOverview.html) - 실시간 카메라
- [Camera Systems in Games](https://www.gamedeveloper.com/design/game-cameras) - 게임 내 카메라 시스템

---

**다음 학습**: [모드 로더 구현](../scenario-mod-loader/) | **이전**: [시각 효과 수정](../scenario-visual-effects/)

**⚡ 완료 예상 시간**: 14-21일 (하루 1-2시간 투자 기준)