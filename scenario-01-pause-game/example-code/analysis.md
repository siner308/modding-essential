# EldenRing PauseTheGame 모드 코드 분석

## 개요

EldenRing의 PauseTheGame 모드는 온라인 게임에서 실시간 일시정지 기능을 제공하는 대표적인 예제입니다. 이 모드는 메모리 패치 기법의 핵심 개념들을 잘 보여줍니다.

## 📁 파일 구조

```
PauseTheGame/
├── DllMain.cpp           # 메인 로직 및 패치 구현
├── InputTranslation.h    # 키보드/컨트롤러 입력 매핑
└── pause_keybinds.ini    # 설정 파일 (런타임 생성)
```

## 🔍 핵심 기술 분석

### 1. AOB (Array of Bytes) 패턴

```cpp
std::string aob = "0f 84 ? ? ? ? c6 ? ? ? ? ? 00 ? 8d ? ? ? ? ? ? 89 ? ? 89 ? ? ? 8b ? ? ? ? ? ? 85 ? 75";
```

**패턴 해석:**
```assembly
0f 84 ? ? ? ?     ; JE (Jump if Equal) - 상대 주소
c6 ? ? ? ? ? 00   ; MOV byte ptr [address], 0
8d ? ? ? ? ? ?     ; LEA (Load Effective Address)
89 ? ?            ; MOV (register to memory)
89 ? ? ?          ; MOV (register to memory)
8b ? ? ? ? ? ?     ; MOV (memory to register)
85 ? 75           ; TEST + JNZ sequence
```

**와일드카드(?) 사용 이유:**
- 게임 버전별 주소 차이 대응
- 컴파일러 최적화 변수 대응
- 포워드 호환성 확보

### 2. 메모리 패치 메커니즘

#### 패치 대상 코드 (추정)
```assembly
; 게임 내부 로직 (역분석을 통해 추정)
cmp byte ptr [game_state], 0    ; 게임 상태 확인
je skip_game_logic              ; 0f 84 XX XX XX XX
; [게임 업데이트 로직들]
call UpdateWorld
call UpdateAI  
call UpdatePhysics
skip_game_logic:
ret
```

#### 패치 적용 과정
```cpp
void Pause() {
    Log("Paused");
    // JE(0x84) → JNE(0x85) 변경
    ReplaceExpectedBytesAtAddress(patchAddress + 1, "84", "85");
    gameIsPaused = true;
}

void Unpause() {
    Log("Unpaused");
    // JNE(0x85) → JE(0x84) 복원
    ReplaceExpectedBytesAtAddress(patchAddress + 1, "85", "84");
    gameIsPaused = false;
}
```

### 3. 조건부 점프 변환의 효과

#### 원본 로직
```
IF (game_state == PAUSED)
    THEN skip_game_logic
    ELSE execute_game_logic
```

#### 패치 후 로직
```
IF (game_state != PAUSED)  // 조건 반전!
    THEN skip_game_logic
    ELSE execute_game_logic
```

**결과:** 게임 상태 값에 관계없이 반대 동작 수행

### 4. 입력 처리 시스템

#### 키바인드 구조체
```cpp
struct Keybind {
    std::vector<unsigned short> keys;
    bool isControllerKeybind;
};
```

#### 기본 키바인드 설정
```cpp
std::vector<Keybind> pauseKeybinds = {
    { { keycodes.at("p") }, false },                                    // P키
    { { controllerKeycodes.at("lthumbpress"), 
        controllerKeycodes.at("xa") }, true }                          // L스틱+A버튼
};
```

#### 입력 감지 루프
```cpp
while (true) {
    auto* keybinds = gameIsPaused ? &unpauseKeybinds : &pauseKeybinds;
    
    for (Keybind keybind : *keybinds) {
        if (AreKeysPressed(keybind.keys, false, keybind.isControllerKeybind)) {
            gameIsPaused ? Unpause() : Pause();
            break;
        }
    }
    
    Sleep(5);  // CPU 사용률 제한
}
```

## 🧠 설계 패턴 분석

### 1. 상태 기반 토글 패턴

```cpp
// 단순한 불리언 토글이 아닌 메모리 패치 기반
bool gameIsPaused = false;

void TogglePause() {
    if (gameIsPaused) {
        // 메모리에서 원본 바이트 복원
        RestoreOriginalBytes();
    } else {
        // 메모리에 패치 바이트 적용  
        ApplyPatchBytes();
    }
    gameIsPaused = !gameIsPaused;
}
```

### 2. 설정 시스템 패턴

```cpp
void ReadConfig() {
    INIFile config(GetModFolderPath() + "\\pause_keybinds.ini");
    INIStructure ini;
    
    if (config.read(ini)) {
        // 기존 설정 로드
        pauseKeybinds = TranslateInput(ini["keybinds"].get("pause_keys"));
        unpauseKeybinds = TranslateInput(ini["keybinds"].get("unpause_keys"));
    } else {
        // 기본 설정으로 파일 생성
        ini["keybinds"]["pause_keys"] = "p, lthumbpress+xa";
        ini["keybinds"]["unpause_keys"] = "p, lthumbpress+xa";
        config.write(ini, true);
    }
}
```

### 3. 에러 처리 패턴

```cpp
DWORD WINAPI MainThread(LPVOID lpParam) {
    Log("Activating PauseTheGame...");
    
    // AOB 스캔 실패 시 안전한 종료
    patchAddress = AobScan(aob);
    if (patchAddress == 0) {
        Log("Failed to find patch location");
        return 1;  // 모드 로드 실패
    }
    
    // 설정 파일 로드
    ReadConfig();
    
    // 메인 루프 (무한 루프)
    while (true) {
        // 입력 처리 로직
        Sleep(5);
    }
    
    CloseLog();
    return 0;
}
```

## 🔬 기술적 세부사항

### 1. 메모리 보호 우회

ModUtils의 `ReplaceExpectedBytesAtAddress` 함수는 다음 과정을 거칩니다:

```cpp
bool ReplaceExpectedBytesAtAddress(uintptr_t address, 
                                  std::string expectedBytes, 
                                  std::string newBytes) {
    // 1. 메모리 보호 해제
    ToggleMemoryProtection(false, address, newBytes.length());
    
    // 2. 현재 바이트 검증
    if (!VerifyCurrentBytes(address, expectedBytes)) {
        return false;
    }
    
    // 3. 새로운 바이트 적용
    ApplyNewBytes(address, newBytes);
    
    // 4. 메모리 보호 복원
    ToggleMemoryProtection(true, address, newBytes.length());
    
    return true;
}
```

### 2. 플랫폼별 입력 처리

#### Windows API 키보드 처리
```cpp
bool IsKeyPressed(unsigned short key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}
```

#### XInput 컨트롤러 처리
```cpp
bool IsControllerButtonPressed(unsigned short button) {
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state;
        if (XInputGetState(i, &state) == ERROR_SUCCESS) {
            return (state.Gamepad.wButtons & button) != 0;
        }
    }
    return false;
}
```

### 3. 동적 주소 해결

```cpp
uintptr_t AobScan(std::string pattern) {
    // 1. 프로세스 베이스 주소 획득
    uintptr_t baseAddress = GetProcessBaseAddress(GetCurrentProcessId());
    
    // 2. 메모리 영역 순회
    MEMORY_BASIC_INFORMATION memInfo;
    uintptr_t currentAddress = baseAddress;
    
    while (VirtualQuery((void*)currentAddress, &memInfo, sizeof(memInfo))) {
        // 3. 실행 가능한 메모리 영역에서만 검색
        if (IsExecutableMemory(memInfo)) {
            uintptr_t found = SearchPattern(currentAddress, pattern);
            if (found != 0) return found;
        }
        currentAddress += memInfo.RegionSize;
    }
    
    return 0;  // 패턴을 찾지 못함
}
```

## 📊 성능 및 최적화

### 1. CPU 사용률 최적화

```cpp
// 메인 루프에서 CPU 사용률 제한
while (true) {
    ProcessInput();
    Sleep(5);  // 200 FPS 제한 (1000ms / 5ms = 200)
}
```

### 2. 메모리 접근 최적화

```cpp
// 캐시된 키 상태로 중복 API 호출 방지
static std::map<unsigned short, bool> keyStates;
static DWORD lastUpdateTime = 0;

bool GetCachedKeyState(unsigned short key) {
    DWORD currentTime = GetTickCount();
    if (currentTime - lastUpdateTime > 16) {  // ~60 FPS 업데이트
        UpdateAllKeyStates();
        lastUpdateTime = currentTime;
    }
    return keyStates[key];
}
```

### 3. 패턴 매칭 최적화

```cpp
// Boyer-Moore 알고리즘을 사용한 빠른 패턴 검색
class PatternMatcher {
private:
    std::vector<int> buildSkipTable(const std::vector<uint8_t>& pattern);
    
public:
    uintptr_t search(uintptr_t start, size_t length, 
                    const std::vector<uint8_t>& pattern);
};
```

## 🎯 학습 포인트

### 1. 핵심 개념
- **조건부 점프 조작**: JE ↔ JNE 변환으로 로직 반전
- **AOB 패턴 매칭**: 게임 업데이트 대응 가능한 패턴 설계
- **메모리 권한 관리**: 안전한 런타임 패치 적용

### 2. 실용적 기법
- **설정 시스템**: INI 파일을 통한 사용자 커스터마이징
- **입력 추상화**: 키보드/컨트롤러 통합 처리
- **에러 복구**: 패치 실패 시 안전한 폴백

### 3. 고급 최적화
- **CPU 효율성**: 폴링 주기 최적화
- **메모리 효율성**: 캐싱을 통한 API 호출 최소화
- **알고리즘 효율성**: 고속 패턴 매칭 알고리즘

## 🔄 변형 및 확장 아이디어

### 1. 기능 확장
```cpp
// 시간 배율 조정 (슬로우 모션 효과)
void SetTimeScale(float scale) {
    // 게임 타이머 관련 메모리 주소에 배율 적용
}

// 조건부 일시정지 (특정 상황에서만)
void ConditionalPause() {
    if (IsInBattle() || IsInCutscene()) {
        // 특정 조건에서만 일시정지 허용
    }
}
```

### 2. UI 통합
```cpp
// 시각적 피드백
void ShowPauseOverlay() {
    // DirectX/OpenGL 오버레이로 일시정지 상태 표시
}

// 설정 GUI
void ShowConfigWindow() {
    // ImGui를 사용한 실시간 설정 변경
}
```

### 3. 다른 게임 엔진 지원
```cpp
// Unity 게임 대응
class UnityPauseSystem {
    // Time.timeScale 조작
    void SetTimeScale(float scale);
};

// Unreal Engine 대응  
class UnrealPauseSystem {
    // World->GetWorld()->GetTimeSeconds() 조작
    void PauseWorldTime();
};
```

---

**💡 핵심 통찰**: 

이 모드는 단순해 보이지만 게임 모딩의 핵심 기법들을 모두 포함하고 있습니다:
- 동적 코드 분석 (AOB 스캔)
- 런타임 메모리 패치
- 시스템 권한 관리
- 사용자 인터페이스 설계
- 에러 처리 및 복구

이러한 패턴들은 다른 모든 게임 모딩 프로젝트의 기초가 됩니다.