# 🟢 Scenario 02: FPS 제한 해제 모딩

**난이도**: 초급-중급 | **학습 시간**: 1-2주 | **접근법**: 부동소수점 수치 조작

60FPS 제한이 걸린 게임을 120FPS, 144FPS 또는 무제한으로 해제하는 모딩 기법을 학습합니다.

## 📖 학습 목표

이 시나리오를 완료하면 다음을 할 수 있게 됩니다:

- [ ] 게임 내 FPS 제한 메커니즘 이해하기
- [ ] 부동소수점 값을 메모리에서 찾고 수정하기
- [ ] DeltaTime과 프레임레이트 관계 파악하기
- [ ] 안전한 FPS 조정 기법 적용하기
- [ ] 게임별 FPS 언락 패턴 분석하기

## 🎯 최종 결과물

완성된 모드의 기능:
- **FPS 제한 완전 해제** 또는 **원하는 수치로 설정**
- **실시간 FPS 조정** (키보드 단축키)
- **안전 모드** (게임 안정성 확보)
- **설정 저장** (게임 재시작 시에도 유지)

## 🔧 FPS 제한의 원리

### 1. 게임 루프와 프레임레이트
```cpp
// 일반적인 게임 루프
while (gameRunning) {
    float deltaTime = GetDeltaTime(); // 이전 프레임과의 시간 차이
    
    UpdateGame(deltaTime);           // 게임 로직 업데이트
    RenderFrame();                   // 화면 렌더링
    
    // FPS 제한 (60FPS = 16.67ms)
    if (deltaTime < targetFrameTime) {
        Sleep(targetFrameTime - deltaTime);
    }
}
```

### 2. FPS 제한 방식들
```
FPS 제한 구현 방법:
├── VSync 사용 - GPU/모니터 동기화
├── Sleep 함수 - CPU 대기 시간 추가
├── DeltaTime 고정 - 시간 값 직접 제한
└── 엔진 내장 - 게임 엔진의 내부 제한
```

### 3. 메모리에서 찾을 수 있는 값들
```cpp
// 찾아야 할 핵심 값들
float targetFPS = 60.0f;           // 목표 FPS
float deltaTime = 0.0166667f;      // 1/60 초
float frameTime = 16.667f;         // 밀리초 단위
int vsyncEnabled = 1;              // VSync 활성화 플래그
```

## 🎮 실제 게임 예제 분석

### EldenRing FPS 언락 분석

```cpp
// EldenRing의 FPS 제한 패턴
// 메모리 주소에서 60.0f 값을 찾아서 수정

class EldenRingFPSUnlock {
private:
    uintptr_t fpsLimitAddress = 0;
    float originalFPS = 60.0f;
    float targetFPS = 120.0f;

public:
    bool FindFPSLimit() {
        // 60.0f 값을 메모리에서 검색
        std::vector<uintptr_t> addresses = ScanForFloat(60.0f);
        
        for (auto addr : addresses) {
            // 주변 메모리 패턴 확인
            if (ValidateFPSAddress(addr)) {
                fpsLimitAddress = addr;
                return true;
            }
        }
        return false;
    }
    
    bool UnlockFPS() {
        if (fpsLimitAddress == 0) return false;
        
        // 60.0f → 120.0f로 변경
        return WriteFloat(fpsLimitAddress, targetFPS);
    }
};
```

### Dark Souls 시리즈 패턴
```
Dark Souls FPS 언락 특징:
- 물리 엔진과 연동되어 있음
- FPS 변경 시 게임 속도도 변경됨  
- 별도의 속도 보정이 필요
- 온라인에서 밴 위험 높음
```

### Unity 게임 일반 패턴
```csharp
// Unity 게임의 FPS 설정
Application.targetFrameRate = 60;  // C# 스크립트에서
Time.fixedDeltaTime = 0.02f;      // 50FPS 물리 업데이트

// 메모리에서 찾을 수 있는 값들:
// Application.targetFrameRate (int)
// Time.fixedDeltaTime (float)
```

## 🔍 실습: Cheat Engine으로 FPS 값 찾기

### 1단계: 기본 스캔
```bash
1. Cheat Engine 실행
2. 게임 프로세스 연결
3. Value Type: Float
4. Scan Type: Exact Value
5. Value: 60 입력 후 First Scan
```

### 2단계: 값 확인
```bash
1. 게임 설정에서 FPS 제한 변경 (가능한 경우)
2. Changed value 스캔
3. 결과가 1-3개 나올 때까지 반복
4. 각 주소의 값을 임시로 수정해서 테스트
```

### 3단계: 주소 검증
```bash
올바른 FPS 주소 특징:
✅ 값 변경 시 즉시 FPS 변화 확인
✅ 게임 크래시 없음
✅ 다른 게임 요소에 영향 없음
❌ 게임 속도가 같이 변하면 잘못된 주소
```

## 💻 FPS 언락 모드 구현

### C++ 구현 예제

```cpp
// FPSUnlocker.h
#pragma once
#include <Windows.h>
#include <vector>
#include <iostream>

class FPSUnlocker {
private:
    HANDLE processHandle;
    DWORD processId;
    uintptr_t fpsAddress;
    float originalFPS;
    bool isUnlocked;

public:
    FPSUnlocker();
    ~FPSUnlocker();
    
    bool Initialize(const std::wstring& processName);
    bool FindFPSLimit();
    bool SetFPS(float targetFPS);
    bool RestoreFPS();
    float GetCurrentFPS();
    
private:
    std::vector<uintptr_t> ScanForFloat(float value);
    bool ValidateAddress(uintptr_t address);
    bool WriteFloat(uintptr_t address, float value);
    float ReadFloat(uintptr_t address);
};

// FPSUnlocker.cpp
#include "FPSUnlocker.h"
#include <TlHelp32.h>

FPSUnlocker::FPSUnlocker() : processHandle(nullptr), processId(0), 
                             fpsAddress(0), originalFPS(60.0f), isUnlocked(false) {}

FPSUnlocker::~FPSUnlocker() {
    if (isUnlocked) {
        RestoreFPS();
    }
    if (processHandle) {
        CloseHandle(processHandle);
    }
}

bool FPSUnlocker::Initialize(const std::wstring& processName) {
    // 프로세스 찾기
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    
    bool found = false;
    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                processId = pe32.th32ProcessID;
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &pe32));
    }
    
    CloseHandle(snapshot);
    if (!found) return false;
    
    // 프로세스 핸들 열기
    processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    return processHandle != nullptr;
}

std::vector<uintptr_t> FPSUnlocker::ScanForFloat(float value) {
    std::vector<uintptr_t> results;
    
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t address = 0;
    
    while (VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & PAGE_READWRITE || mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            
            std::vector<char> buffer(mbi.RegionSize);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, mbi.BaseAddress, 
                                buffer.data(), mbi.RegionSize, &bytesRead)) {
                
                for (size_t i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
                    float* floatPtr = reinterpret_cast<float*>(&buffer[i]);
                    if (abs(*floatPtr - value) < 0.001f) {
                        results.push_back((uintptr_t)mbi.BaseAddress + i);
                    }
                }
            }
        }
        address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    
    return results;
}

bool FPSUnlocker::FindFPSLimit() {
    std::cout << "FPS 제한 값 검색 중..." << std::endl;
    
    // 60.0f 값 검색
    auto addresses = ScanForFloat(60.0f);
    std::cout << "60.0f 값 " << addresses.size() << "개 발견" << std::endl;
    
    // 주소 검증
    for (auto addr : addresses) {
        if (ValidateAddress(addr)) {
            fpsAddress = addr;
            originalFPS = ReadFloat(addr);
            std::cout << "FPS 주소 발견: 0x" << std::hex << addr << std::endl;
            return true;
        }
    }
    
    return false;
}

bool FPSUnlocker::ValidateAddress(uintptr_t address) {
    // 현재 값 읽기
    float currentValue = ReadFloat(address);
    if (abs(currentValue - 60.0f) > 0.1f) return false;
    
    // 임시로 값 변경해서 테스트
    if (!WriteFloat(address, 120.0f)) return false;
    Sleep(100);
    
    // 원본 복원
    WriteFloat(address, currentValue);
    
    return true;
}

bool FPSUnlocker::SetFPS(float targetFPS) {
    if (fpsAddress == 0) return false;
    
    if (WriteFloat(fpsAddress, targetFPS)) {
        isUnlocked = true;
        std::cout << "FPS 제한 해제: " << targetFPS << "FPS" << std::endl;
        return true;
    }
    
    return false;
}

bool FPSUnlocker::RestoreFPS() {
    if (fpsAddress == 0 || !isUnlocked) return false;
    
    if (WriteFloat(fpsAddress, originalFPS)) {
        isUnlocked = false;
        std::cout << "FPS 제한 복원: " << originalFPS << "FPS" << std::endl;
        return true;
    }
    
    return false;
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

// main.cpp - 사용 예시
#include "FPSUnlocker.h"
#include <iostream>
#include <string>

int main() {
    std::cout << "=== FPS Unlocker ===" << std::endl;
    
    FPSUnlocker unlocker;
    
    // EldenRing 프로세스 연결
    if (!unlocker.Initialize(L"eldenring.exe")) {
        std::cout << "게임 프로세스를 찾을 수 없습니다." << std::endl;
        return 1;
    }
    
    // FPS 제한 찾기
    if (!unlocker.FindFPSLimit()) {
        std::cout << "FPS 제한을 찾을 수 없습니다." << std::endl;
        return 1;
    }
    
    // 사용자 입력 처리
    std::string input;
    while (true) {
        std::cout << "\n명령어 입력:" << std::endl;
        std::cout << "1. 120FPS 설정" << std::endl;
        std::cout << "2. 144FPS 설정" << std::endl;
        std::cout << "3. 무제한 (300FPS)" << std::endl;
        std::cout << "4. 원본 복원" << std::endl;
        std::cout << "5. 종료" << std::endl;
        
        std::getline(std::cin, input);
        
        if (input == "1") {
            unlocker.SetFPS(120.0f);
        } else if (input == "2") {
            unlocker.SetFPS(144.0f);
        } else if (input == "3") {
            unlocker.SetFPS(300.0f);
        } else if (input == "4") {
            unlocker.RestoreFPS();
        } else if (input == "5") {
            break;
        }
    }
    
    return 0;
}
```

## 🔧 고급 기법: 동적 FPS 조정

### 실시간 FPS 변경
```cpp
class DynamicFPSController {
private:
    FPSUnlocker* unlocker;
    float currentFPS;
    bool hotkeyEnabled;

public:
    void RegisterHotkeys() {
        // F1: FPS 증가 (+10)
        RegisterHotKey(nullptr, 1, 0, VK_F1);
        
        // F2: FPS 감소 (-10)
        RegisterHotKey(nullptr, 2, 0, VK_F2);
        
        // F3: 60FPS 복원
        RegisterHotKey(nullptr, 3, 0, VK_F3);
    }
    
    void ProcessMessages() {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            if (msg.message == WM_HOTKEY) {
                switch (msg.wParam) {
                    case 1: // F1 - FPS 증가
                        currentFPS = min(currentFPS + 10.0f, 300.0f);
                        unlocker->SetFPS(currentFPS);
                        break;
                        
                    case 2: // F2 - FPS 감소  
                        currentFPS = max(currentFPS - 10.0f, 30.0f);
                        unlocker->SetFPS(currentFPS);
                        break;
                        
                    case 3: // F3 - 복원
                        unlocker->RestoreFPS();
                        break;
                }
            }
        }
    }
};
```

## ⚠️ 주의사항 및 한계

### 1. 게임별 특수 고려사항

**EldenRing / Dark Souls:**
```
- 물리 엔진이 프레임레이트와 연동
- 높은 FPS에서 물리 버그 발생 가능
- 온라인에서 동기화 문제 발생
- 권장: 60-120FPS 범위 내에서만 사용
```

**Unity 게임:**
```
- Time.timeScale 영향 확인 필요
- fixedDeltaTime과 별도 관리
- UI 요소가 FPS에 영향받을 수 있음
- 각 게임마다 다른 최적화 필요
```

**언리얼 엔진 게임:**
```
- t.MaxFPS 콘솔 명령어 존재
- 엔진 설정에서 제어 가능한 경우 많음
- GameUserSettings.ini 파일 수정으로도 가능
- 메모리 패치보다 설정 파일 수정 권장
```

### 2. 안전성 검증
```cpp
bool SafeFPSCheck(float newFPS) {
    // 1. 합리적 범위 확인
    if (newFPS < 30.0f || newFPS > 300.0f) {
        return false;
    }
    
    // 2. 게임 상태 확인
    if (IsGameInCutscene() || IsGameLoading()) {
        return false; // 특정 상황에서는 변경 금지
    }
    
    // 3. 시스템 성능 확인
    if (GetCPUUsage() > 90.0f || GetGPUUsage() > 95.0f) {
        return false; // 시스템 부하가 높으면 변경 금지
    }
    
    return true;
}
```

## 📊 성능 모니터링

### FPS 측정 및 표시
```cpp
class FPSMonitor {
private:
    std::chrono::high_resolution_clock::time_point lastTime;
    int frameCount;
    float currentFPS;

public:
    void Update() {
        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastTime).count();
        
        if (duration >= 1000) { // 1초마다 업데이트
            currentFPS = frameCount * 1000.0f / duration;
            frameCount = 0;
            lastTime = currentTime;
            
            std::cout << "Current FPS: " << currentFPS << std::endl;
        }
    }
    
    float GetFPS() const { return currentFPS; }
};
```

## 🚀 심화 실습 과제

### 과제 1: 게임별 FPS 언락 (초급)
- [ ] **Skyrim SE**: Creation Engine FPS 언락
- [ ] **Fallout 4**: 유사한 엔진 패턴 적용
- [ ] **The Witcher 3**: REDengine FPS 제한 해제

### 과제 2: GUI 인터페이스 (중급)
- [ ] **슬라이더**: 30-300 FPS 범위 조정
- [ ] **프리셋 버튼**: 60/120/144/무제한
- [ ] **실시간 모니터링**: 현재 FPS 표시

### 과제 3: 자동화 기능 (고급)
- [ ] **게임 감지**: 자동으로 지원 게임 탐지
- [ ] **프로필 관리**: 게임별 FPS 설정 저장
- [ ] **안전 모드**: 자동 복원 및 예외 처리

## 🔗 관련 자료

- [PC Gaming Wiki - FPS caps](https://www.pcgamingwiki.com/wiki/Glossary:Frame_rate_(FPS)#Frame_rate_caps) - PC 게이밍 위키 - FPS 제한
- [Cheat Engine Tutorial](https://cheatengine.org/tutorials.php) - 치트 엔진 튜토리얼
- [Game Engine Architecture](https://www.gameenginebook.com/) - 게임 엔진 아키텍처

## 💡 트러블슈팅

### Q: FPS 변경 후 게임이 빨라졌어요
```
A: 게임 로직이 프레임레이트에 종속된 경우입니다.
해결책:
1. deltaTime 기반 업데이트로 변경 필요
2. 물리 업데이트와 렌더링 분리
3. 해당 게임은 FPS 언락 권장하지 않음
```

### Q: 특정 FPS에서만 크래시가 발생해요
```
A: 하드웨어나 엔진 한계일 수 있습니다.
확인사항:
1. GPU/CPU 온도 및 사용률
2. VRAM 부족 여부
3. 게임 엔진의 최대 지원 FPS
```

### Q: 온라인 게임에서 사용해도 되나요?
```
A: 절대 권장하지 않습니다.
위험요소:
1. 서버 동기화 문제
2. 안티치트 시스템 탐지
3. 계정 밴 위험
4. 다른 플레이어에게 불공정함
```

---

**다음 학습**: [Scenario 03: 시각 효과 수정](../scenario-03-visual-effects/) | **이전**: [Scenario 01: 게임 일시정지 구현](../scenario-01-pause-game/)

**⚡ 완료 예상 시간**: 7-10일 (하루 1-2시간 투자 기준)