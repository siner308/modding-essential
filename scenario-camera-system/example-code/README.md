# Camera System Example Code

게임의 카메라 시스템을 완전히 제어하는 고급 C++ 구현 예제입니다.

## 📁 파일 구조

```
example-code/
├── CameraSystem.h         # 카메라 시스템 헤더 (모든 클래스 정의)
├── CameraSystem.cpp       # 카메라 시스템 구현
├── main.cpp               # 메인 애플리케이션 및 UI
├── CMakeLists.txt         # CMake 빌드 스크립트
└── README.md              # 이 파일
```

## 🚀 빌드 방법

### 필수 요구사항

1. **Visual Studio 2019 이상** - MSVC 컴파일러
2. **Windows SDK** - DirectX Math 라이브러리 포함
3. **CMake 3.16 이상** - 빌드 시스템

### Windows (Visual Studio)

```bash
# 프로젝트 빌드
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release

# 또는 제공된 배치 파일 사용
build.bat
```

### 라이브러리 의존성

```cmake
# 필요한 Windows 라이브러리 (자동 링크)
- kernel32.lib    # 기본 Windows API
- user32.lib      # 사용자 인터페이스 API
- psapi.lib       # 프로세스 API

# DirectX Math는 헤더 전용 라이브러리 (Windows SDK 포함)
```

## 💻 사용법

### 기본 사용

```cpp
#include "CameraSystem.h"

int main() {
    // 1. 카메라 시스템 초기화
    CameraSystem camera;
    
    if (!camera.Initialize(L"eldenring.exe")) {
        std::cout << "초기화 실패" << std::endl;
        return -1;
    }
    
    // 2. 자유 카메라 활성화
    camera.EnableFreeCamera(true);
    
    // 3. 메인 루프에서 업데이트
    while (running) {
        camera.Update();  // 입력 처리 및 전환 업데이트
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // 4. 정리
    camera.Shutdown();
    return 0;
}
```

### 자유 카메라 제어

```cpp
// 자유 카메라 활성화
camera.EnableFreeCamera(true);

// 카메라 속도 설정
camera.SetFreeCameraSpeed(5.0f);

// 수동 위치 설정
camera.SetCameraPosition({100.0f, 50.0f, 200.0f});

// 수동 회전 설정 (라디안)
XMFLOAT3 rotation = {
    CameraUtils::DegreesToRadians(15.0f),  // 피치
    CameraUtils::DegreesToRadians(45.0f),  // 요
    0.0f                                   // 롤
};
camera.SetCameraRotation(rotation);
```

### FOV 조정

```cpp
// 현재 FOV 확인
float currentFOV;
if (camera.GetFOV(currentFOV)) {
    std::cout << "현재 FOV: " << currentFOV << "도" << std::endl;
}

// FOV 설정
camera.SetFOV(90.0f);  // 90도 FOV

// FOV 증감
camera.AdjustFOV(10.0f);   // +10도
camera.AdjustFOV(-5.0f);   // -5도

// FOV 제한 설정
camera.SetFOVLimits(30.0f, 120.0f);  // 30도~120도
```

### 카메라 전환 및 애니메이션

```cpp
// 부드러운 카메라 전환
CameraState targetState;
targetState.position = {0.0f, 10.0f, 0.0f};
targetState.rotation = {0.0f, 0.0f, 0.0f};
targetState.fov = 75.0f;

camera.StartCameraTransition(targetState, 2.0f, CameraTransition::EaseType::EaseInOut);

// 전환 상태 확인
if (camera.IsTransitionActive()) {
    std::cout << "카메라 전환 중..." << std::endl;
}

// 전환 중단
camera.StopTransition();
```

### 시네마틱 카메라 시퀀스

```cpp
// 시네마틱 카메라 생성
CinematicCamera cinematicCamera(&camera);

// 웨이포인트 추가
CameraState waypoint1, waypoint2, waypoint3;
// ... 웨이포인트 설정 ...

cinematicCamera.AddWaypoint(waypoint1, 2.0f); // 2초 지속
cinematicCamera.AddWaypoint(waypoint2, 3.0f); // 3초 지속
cinematicCamera.AddWaypoint(waypoint3, 1.5f); // 1.5초 지속

// 시퀀스 재생
cinematicCamera.SetLooping(true);  // 반복 재생
cinematicCamera.Play();

// 메인 루프에서 업데이트 필요
while (running) {
    cinematicCamera.Update();
    // ...
}
```

### 포토 모드

```cpp
// 포토 모드 생성
PhotoMode photoMode(&camera);

// 포토 모드 진입
photoMode.EnterPhotoMode();

// 포토 모드 설정
photoMode.SetDepthOfField(2.5f);     // 피사계 심도
photoMode.SetExposure(0.5f);         // 노출
photoMode.SetOrthographicMode(true); // 직교 투영

// 스크린샷 촬영
photoMode.TakeScreenshot("my_screenshot");

// 포토 모드 종료
photoMode.ExitPhotoMode();
```

## 🎮 지원 게임

### 테스트된 게임들

- ✅ **Elden Ring** - 완전 지원 (모든 기능 작동)
- ✅ **Dark Souls III** - 완전 지원
- ✅ **Skyrim Special Edition** - 완전 지원  
- ✅ **The Witcher 3** - 완전 지원
- ✅ **Cyberpunk 2077** - 부분 지원 (FOV만)
- ⚠️ **온라인 게임** - 안티치트로 인한 제한

### 게임 엔진별 지원

```cpp
// Unreal Engine 게임
- Elden Ring, Dark Souls 시리즈
- Fortnite (오프라인 모드)
- Gears of War 시리즈

// Unity 게임  
- Hearthstone
- Cities: Skylines
- Ori and the Blind Forest

// 커스텀 엔진
- The Witcher 3 (REDengine)
- Skyrim (Creation Engine)
- GTA V (RAGE)
```

## 🔧 핵심 기능

### 1. 메모리 스캔 및 패턴 매칭

```cpp
// 다양한 게임 엔진의 카메라 패턴
const std::vector<uint8_t> UE4_CAMERA_PATTERN = {
    0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00,  // mov rax, [rip+offset]
    0x48, 0x85, 0xC0,                           // test rax, rax  
    0x74, 0x00,                                 // jz short
    0xF3, 0x0F, 0x10, 0x40, 0x00               // movss xmm0, [rax+offset]
};

// 패턴 스캔으로 카메라 주소 찾기
auto addresses = ScanMemoryPattern(UE4_CAMERA_PATTERN, UE4_CAMERA_MASK);
for (uintptr_t addr : addresses) {
    if (ValidateCameraAddress(addr)) {
        cameraBaseAddress = addr;
        break;
    }
}
```

### 2. 안전한 메모리 접근

```cpp
// 안전성 검증
bool ValidateCameraState(const CameraState& state) {
    // NaN 및 무한값 검사
    if (!isfinite(state.position.x) || !isfinite(state.position.y) || !isfinite(state.position.z)) {
        return false;
    }
    
    // 합리적 범위 확인
    if (abs(state.position.x) > 1000000.0f) {
        return false;
    }
    
    return true;
}

// 안전 모드 설정
camera.SetSafetyMode(true);  // 극한 값 방지
```

### 3. 부드러운 카메라 전환

```cpp
// 다양한 이징 함수 지원
float EaseFunction(float t, CameraTransition::EaseType type) {
    switch (type) {
        case EaseType::Linear:
            return t;
        case EaseType::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
        case EaseType::Elastic:
            return pow(2.0f, -10.0f * t) * sin((t - 0.1f) * 2.0f * PI / 0.4f) + 1.0f;
        // ... 더 많은 이징 함수
    }
}
```

## 🎯 고급 기능

### 1. 실시간 입력 처리

```cpp
// 키보드 및 마우스 입력
void ProcessKeyboard() {
    // WASD 이동
    if (input.keys['W']) {
        freeCameraVelocity += forward * acceleration * deltaTime;
    }
    
    // 속도 조절
    if (input.keys[VK_SHIFT]) {
        currentSpeed *= 2.0f;  // 빠르게
    }
    if (input.keys[VK_CONTROL]) {
        currentSpeed *= 0.5f;  // 천천히
    }
}

// 마우스 룩
void ProcessMouse() {
    if (input.mouseButtons[1]) {  // 우클릭
        rotation.y += input.mouseDeltaX * sensitivity;
        rotation.x += input.mouseDeltaY * sensitivity * (invertY ? 1.0f : -1.0f);
    }
}
```

### 2. 수학 유틸리티

```cpp
// 오일러 각을 방향 벡터로 변환
XMFLOAT3 EulerToDirection(const XMFLOAT3& euler) {
    float pitch = euler.x, yaw = euler.y;
    return {
        cos(pitch) * cos(yaw),
        sin(pitch),
        cos(pitch) * sin(yaw)
    };
}

// 벡터 정규화
XMFLOAT3 Normalize(const XMFLOAT3& v) {
    float length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return {v.x / length, v.y / length, v.z / length};
}
```

### 3. 카메라 프리셋 시스템

```cpp
// 프리셋 저장 (JSON 파일로 저장 가능)
void SaveCameraPreset(const std::string& name) {
    CameraState currentState;
    GetCameraState(currentState);
    
    // 파일에 저장하는 로직
    std::ofstream file(name + ".json");
    // JSON 형태로 저장...
}

// 프리셋 로드
bool LoadCameraPreset(const std::string& name) {
    std::ifstream file(name + ".json");
    if (!file.is_open()) return false;
    
    // JSON에서 로드하여 카메라 상태 복원
    // ...
}
```

## ⚠️ 주의사항

### 시스템 요구사항

1. **관리자 권한 필수**: 메모리 접근을 위해 필요
2. **Windows 10/11**: DirectX Math 라이브러리 필요
3. **64비트 시스템**: 대부분의 최신 게임이 64비트
4. **충분한 메모리**: 게임과 도구 동시 실행

### 안전 가이드라인

```cpp
// 안전 모드 항상 활성화 권장
camera.SetSafetyMode(true);

// FOV 제한 설정
camera.SetFOVLimits(30.0f, 120.0f);  // 극단적 FOV 방지

// 원본 상태 백업
CameraState originalState;
camera.GetCameraState(originalState);  // 자동으로 백업됨
```

### 호환성 문제

**Q: 카메라가 움직이지 않아요**
```
A: 다음을 확인하세요:
1. 자유 카메라가 활성화되었는가? (EnableFreeCamera(true))
2. 게임이 일시정지 상태가 아닌가?
3. 다른 카메라 모드(컷신 등)가 활성화되지 않았는가?
4. 메모리 주소가 올바르게 찾아졌는가?
```

**Q: FOV 변경이 안 돼요**
```
A: 문제 해결:
1. FOV 주소를 찾을 수 없는 경우 - 게임별 패턴 추가 필요
2. 게임이 VSync로 FOV를 덮어쓰는 경우 - 지속적 업데이트 필요
3. 게임 설정에서 FOV가 고정된 경우 - 다른 메모리 위치 탐색
```

**Q: 게임이 크래시돼요**
```
A: 안전 조치:
1. 안전 모드 활성화 (SetSafetyMode(true))
2. 극단적 위치로 이동 금지
3. 카메라 상태 검증 강화
4. 원본 상태 즉시 복원 (RestoreOriginalCamera())
```

## 📊 성능 및 최적화

### 메모리 사용량

```cpp
// 경량화된 구조체 사용
struct CameraState {
    XMFLOAT3 position;    // 12 bytes
    XMFLOAT3 rotation;    // 12 bytes  
    float fov;            // 4 bytes
    // 총 28 bytes (+ 패딩)
};

// 효율적인 메모리 접근
template<typename T>
bool ReadValue(uintptr_t address, T& value) {
    SIZE_T bytesRead;
    return ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(T), &bytesRead) 
           && bytesRead == sizeof(T);
}
```

### CPU 사용량 최적화

```cpp
// 적응형 업데이트 주기
void Update() {
    auto now = std::chrono::steady_clock::now();
    deltaTime = std::chrono::duration<float>(now - lastUpdate).count();
    
    // 60 FPS 제한
    if (deltaTime < 0.016f) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }
    
    lastUpdate = now;
    // 실제 업데이트 로직...
}
```

## 🔗 관련 자료

- [메모리 스캔 고급 기법](../../getting-started/advanced-memory-scanning.md)
- [DirectX Math 라이브러리 가이드](../../getting-started/directx-math-guide.md)
- [카메라 시스템 수정](../README.md)
- [게임 엔진별 카메라 구조](../../advanced-topics/camera-structures.md)

## 🚀 확장 기능

### 카메라 추적 시스템

```cpp
// 특정 객체 추적
CameraTracker tracker(&camera);
tracker.SetTarget(playerAddress);
tracker.SetTrackingMode(CameraTracker::TrackingMode::ThirdPerson);

// 궤도 카메라
tracker.SetTrackingMode(CameraTracker::TrackingMode::Orbit);
tracker.GetTrackingSettings().orbitSpeed = 1.0f;
```

### VR 지원

```cpp
// VR 카메라 설정 (확장 가능)
struct VRCameraState : public CameraState {
    XMFLOAT3 leftEyeOffset;
    XMFLOAT3 rightEyeOffset;
    float interpupillaryDistance;
};
```

### 네트워크 동기화

```cpp
// 멀티플레이어 카메라 동기화 (개념)
class NetworkedCamera {
    void BroadcastCameraState(const CameraState& state);
    void ReceiveCameraState(const CameraState& state, PlayerID player);
};
```

---

**⚡ 빌드 후 `build/bin/Release/CameraSystem.exe`를 관리자 권한으로 실행하세요!**

**🎮 게임을 먼저 실행한 후 카메라 시스템을 시작하면 실시간으로 카메라를 제어할 수 있습니다.**