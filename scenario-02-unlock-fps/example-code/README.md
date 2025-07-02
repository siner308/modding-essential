# FPS Unlocker Example Code

EldenRing과 다른 게임들의 FPS 제한을 해제하는 완전한 C++ 구현 예제입니다.

## 📁 파일 구조

```
example-code/
├── FPSUnlocker.h          # 메인 FPS 언락 클래스 헤더
├── FPSUnlocker.cpp        # 구현 파일
├── main.cpp               # 사용 예제 및 GUI
├── CMakeLists.txt         # CMake 빌드 스크립트
└── README.md              # 이 파일
```

## 🚀 빌드 방법

### Windows (Visual Studio)

```bash
# 방법 1: CMake GUI 사용
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release

# 방법 2: 명령줄 (개발자 명령 프롬프트에서)
mkdir build && cd build
cmake ..
msbuild FPSUnlocker.sln /p:Configuration=Release
```

### Windows (MinGW)

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## 💻 사용법

### 기본 사용

```cpp
#include "FPSUnlocker.h"

// 1. FPS Unlocker 초기화
FPSUnlocker unlocker;
if (!unlocker.Initialize(L"eldenring.exe")) {
    std::cout << "초기화 실패" << std::endl;
    return;
}

// 2. FPS 제한 찾기
if (!unlocker.FindFPSLimit()) {
    std::cout << "FPS 제한을 찾을 수 없음" << std::endl;
    return;
}

// 3. FPS 설정
unlocker.SetFPS(120.0f);  // 120 FPS로 설정
unlocker.SetFPS(0.0f);    // 무제한 FPS

// 4. 원본 복원
unlocker.RestoreFPS();
```

### 고급 기능 사용

```cpp
// 핫키 지원이 포함된 고급 컨트롤러
AdvancedFPSController controller(&unlocker);
controller.EnableHotkeys();

// 메인 루프에서 메시지 처리
while (running) {
    controller.ProcessMessages();  // 핫키 처리
    controller.Update();           // 부드러운 전환
    Sleep(16);                     // ~60Hz 업데이트
}
```

### FPS 모니터링

```cpp
// FPS 측정 도구
FPSUtils::FPSMonitor monitor;

// 프레임마다 호출
monitor.RecordFrame();

// 통계 확인
float avgFPS = monitor.GetAverageFPS();
float minFPS = monitor.GetMinFPS();
float maxFPS = monitor.GetMaxFPS();
```

## 🎮 지원 게임

### 테스트된 게임들

- ✅ **Elden Ring** - 안전 (120 FPS 권장)
- ✅ **Dark Souls III** - 안전 (120 FPS 권장)  
- ✅ **Skyrim SE** - 안전 (144 FPS 권장)
- ✅ **The Witcher 3** - 안전 (무제한 가능)
- ⚠️ **Dark Souls (Original)** - 주의 (물리 버그 가능)

### 게임별 권장 설정

```cpp
// 자동으로 안전한 FPS 값 확인
if (FPSUtils::IsGameFPSChangeSafe(L"eldenring.exe")) {
    float maxSafe = FPSUtils::GetRecommendedMaxFPS(L"eldenring.exe");
    std::cout << "권장 최대 FPS: " << maxSafe << std::endl;
}
```

## 🔧 핵심 기능

### 1. 메모리 스캔

```cpp
// 여러 FPS 값을 동시에 스캔
std::vector<float> commonFPS = {60.0f, 30.0f, 120.0f, 144.0f};
for (float fps : commonFPS) {
    auto addresses = ScanForFloat(fps);
    // 각 주소 검증...
}
```

### 2. 주소 검증

```cpp
bool ValidateAddress(uintptr_t address) {
    float currentValue = ReadFloat(address);
    
    // 합리적 범위 확인
    if (currentValue < 10.0f || currentValue > 1000.0f) {
        return false;
    }
    
    // 쓰기 테스트
    float testValue = currentValue + 1.0f;
    WriteFloat(address, testValue);
    
    // 검증 후 복원
    float readBack = ReadFloat(address);
    WriteFloat(address, currentValue);
    
    return (abs(readBack - testValue) < 0.1f);
}
```

### 3. 안전한 FPS 설정

```cpp
bool SetFPS(float targetFPS) {
    // 범위 검증
    if (targetFPS != 0.0f && (targetFPS < 10.0f || targetFPS > 1000.0f)) {
        return false;
    }
    
    // 무제한 FPS 처리
    float actualFPS = (targetFPS == 0.0f) ? 9999.0f : targetFPS;
    
    return WriteFloat(fpsAddress, actualFPS);
}
```

## 🎯 핫키 시스템

기본 핫키:
- **F1/F2**: FPS 증가/감소 (10 단위)
- **Ctrl+F1/F2**: 프리셋 순환
- **F3**: 원본 FPS 복원

```cpp
// 핫키 등록
RegisterHotKey(messageWindow, 1, 0, VK_F1);        // F1
RegisterHotKey(messageWindow, 2, 0, VK_F2);        // F2
RegisterHotKey(messageWindow, 3, MOD_CONTROL, VK_F1); // Ctrl+F1
```

## ⚠️ 주의사항

### 안전 가이드라인

1. **관리자 권한 필요**: 메모리 접근을 위해 필수
2. **오프라인 모드**: 온라인 게임에서 사용 금지
3. **백업 생성**: 게임 파일 백업 권장
4. **안티치트**: EAC, BattlEye 등 비활성화 필요

### 게임별 제한사항

```cpp
// FromSoftware 게임 (물리 엔진 연동)
if (processName.find(L"eldenring") != std::wstring::npos) {
    maxRecommendedFPS = 120.0f; // 물리 버그 방지
}

// Skyrim (스크립트 타이밍)
if (processName.find(L"skyrim") != std::wstring::npos) {
    maxRecommendedFPS = 144.0f; // 스크립트 안정성
}
```

### 문제 해결

**Q: FPS 주소를 찾을 수 없어요**
```
A: 다음을 확인하세요:
1. 게임이 완전히 로드되었는가?
2. 게임 설정에서 FPS 제한이 활성화되어 있는가?
3. VSync가 비활성화되어 있는가?
4. 게임이 fullscreen 모드인가?
```

**Q: FPS 변경 후 게임이 빨라졌어요**
```
A: 게임 로직이 프레임레이트에 종속된 경우:
1. 더 낮은 FPS 값 시도 (90-120)
2. 게임 엔진별 추가 패치 필요
3. 해당 게임은 FPS 언락 부적합
```

**Q: 특정 FPS에서 크래시가 발생해요**
```
A: 하드웨어 제한일 수 있음:
1. GPU/CPU 온도 확인
2. VRAM 사용량 확인
3. 전력 공급 안정성 확인
4. 점진적 FPS 증가로 테스트
```

## 📊 성능 모니터링

### 실시간 FPS 측정

```cpp
class FPSMonitor {
    void RecordFrame() {
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
};
```

### 통계 정보

- **평균 FPS**: 전체 프레임의 평균
- **최소 FPS**: 가장 느린 프레임
- **최대 FPS**: 가장 빠른 프레임
- **프레임타임 분산**: 안정성 지표

## 🔗 관련 자료

- [메모리 스캔 가이드](../../getting-started/memory-scanning-guide.md)
- [안전한 개발 가이드](../../getting-started/safe-development-guide.md)
- [Scenario 02: FPS 제한 해제](../README.md)

---

**⚡ 빌드 후 `build/bin/Release/FPSUnlocker.exe`를 관리자 권한으로 실행하세요!**