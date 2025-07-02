# 🔧 메모리 패치 방법 완전 가이드

**실전 도구 준비부터 고급 패치 기법까지 단계별 학습**

## 📖 메모리 패치란?

메모리 패치는 실행 중인 프로그램의 메모리 내용을 직접 수정하여 동작을 변경하는 기법입니다.

### 패치 종류
```
메모리 패치 유형:
├── Hot Patch - 런타임 중 즉시 적용
├── Code Cave - 새로운 코드 삽입
├── Inline Hook - 함수 진입점 가로채기
├── Import Table Patch - API 호출 변경
└── VTable Hook - 가상 함수 테이블 수정
```

## 🛠️ 필수 도구 및 환경 설정

### 1. Cheat Engine (핵심 도구)
```bash
다운로드: https://cheatengine.org/
설치 옵션:
✅ Include debugging symbols
✅ Install Lua scripting
✅ Include dissector
❌ Chrome extension (불필요)
```

### 2. x64dbg (고급 디버깅)
```bash
다운로드: https://x64dbg.com/
필수 플러그인:
- xAnalyzer: 자동 분석
- Snowman: 디컴파일러
- ret-sync: IDA Pro 연동
```

### 3. Visual Studio (개발 환경)
```bash
Community 버전 다운로드
워크로드 선택:
✅ C++를 사용한 데스크톱 개발
✅ C++를 사용한 게임 개발
필수 구성요소:
- Windows 11 SDK
- CMake tools
- Git for Windows
```

### 4. 추가 유틸리티
```bash
HxD (Hex Editor): 파일 직접 편집
Process Monitor: 파일/레지스트리 모니터링
API Monitor: API 호출 추적
Dependency Walker: DLL 종속성 분석
```

## 🎯 패치 기법별 실습

### 1. 기본 바이트 패치

#### **EldenRing 일시정지 모드 분석**
```cpp
// 원본 코드 (DllMain.cpp 32라인)
ReplaceExpectedBytesAtAddress(patchAddress + offset, "84", "85");

// 패치 내용:
// JE (Jump if Equal: 84) → JNE (Jump if Not Equal: 85)
```

#### **직접 구현해보기**
```cpp
#include <Windows.h>

bool PatchMemory(uintptr_t address, const char* originalBytes, const char* newBytes) {
    // 1. 메모리 보호 해제
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)address, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // 2. 원본 바이트 확인
    if (*(BYTE*)address != (BYTE)strtol(originalBytes, nullptr, 16)) {
        VirtualProtect((LPVOID)address, 1, oldProtect, &oldProtect);
        return false;
    }
    
    // 3. 새 바이트로 패치
    *(BYTE*)address = (BYTE)strtol(newBytes, nullptr, 16);
    
    // 4. 메모리 보호 복원
    VirtualProtect((LPVOID)address, 1, oldProtect, &oldProtect);
    
    return true;
}

// 사용 예시
uintptr_t patchAddr = 0x140A2B5C1; // AOB 스캔으로 찾은 주소
PatchMemory(patchAddr, "84", "85"); // JE → JNE 패치
```

### 2. Code Cave 기법

#### **새로운 코드 삽입 방법**
```cpp
// 1. 빈 메모리 공간 할당
LPVOID codeSpace = VirtualAlloc(NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

// 2. 새로운 기능 코드 작성
BYTE newCode[] = {
    0x50,                     // push rax
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, address
    0xFF, 0xD0,               // call rax
    0x58,                     // pop rax
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, return_address
    0xFF, 0xE0                // jmp rax
};

// 3. 코드 복사
memcpy(codeSpace, newCode, sizeof(newCode));

// 4. 원본 함수에서 점프
BYTE jumpToCodeCave[] = {
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, codeSpace
    0xFF, 0xE0,               // jmp rax
    0x90, 0x90, 0x90          // nop padding
};
```

### 3. 함수 후킹 (Detour)

#### **Microsoft Detours 사용**
```cpp
#include <detours.h>

// 원본 함수 포인터
static int (WINAPI *TrueMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) = MessageBoxA;

// 후킹된 함수
int WINAPI DetourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    // 사용자 정의 동작
    return TrueMessageBoxA(hWnd, "Hooked Message!", lpCaption, uType);
}

// 후킹 설치
void InstallHook() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueMessageBoxA, DetourMessageBoxA);
    DetourTransactionCommit();
}

// 후킹 제거
void UninstallHook() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueMessageBoxA, DetourMessageBoxA);
    DetourTransactionCommit();
}
```

### 4. VTable 후킹

#### **가상 함수 테이블 조작**
```cpp
class GameEntity {
public:
    virtual void Update() = 0;
    virtual void Render() = 0;
    virtual void Destroy() = 0;
};

// VTable 후킹 함수
void HookVTable(void* objectInstance, int functionIndex, void* newFunction, void** originalFunction) {
    void** vtable = *(void***)objectInstance;
    
    DWORD oldProtect;
    VirtualProtect(&vtable[functionIndex], sizeof(void*), PAGE_READWRITE, &oldProtect);
    
    *originalFunction = vtable[functionIndex];
    vtable[functionIndex] = newFunction;
    
    VirtualProtect(&vtable[functionIndex], sizeof(void*), oldProtect, &oldProtect);
}
```

## 🔍 실전 패치 예제

### 예제 1: 무한 체력 패치
```cpp
// HP 감소 함수 찾기
"48 83 EC 28 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 20 8B 81 ? ? ? ?"

// 패치 방법 1: NOP으로 무력화
void DisableHPDecrease(uintptr_t address) {
    BYTE nopBytes[] = {0x90, 0x90, 0x90, 0x90, 0x90}; // 5개의 NOP
    WriteMemory(address, nopBytes, sizeof(nopBytes));
}

// 패치 방법 2: 항상 증가하도록 변경
void MakeHPIncrease(uintptr_t address) {
    // SUB를 ADD로 변경
    WriteMemory(address, "\x01", 1); // 0x29 (SUB) → 0x01 (ADD)
}
```

### 예제 2: 무한 아이템 패치
```cpp
// 아이템 감소 코드 찾기
"FF 48 ? 8B 48 ? 85 C9 7E"

// 감소 대신 증가하도록 패치
void PatchItemUsage(uintptr_t address) {
    // DEC를 INC로 변경
    WriteMemory(address, "\x40", 1); // 0x48 (DEC) → 0x40 (INC)
}
```

### 예제 3: 스피드 해킹
```cpp
// 이동 속도 제한 코드 찾기
void PatchMovementSpeed(uintptr_t speedLimitAddress) {
    // 원본: 최대 속도 100.0f
    float newMaxSpeed = 500.0f;
    WriteMemory(speedLimitAddress, &newMaxSpeed, sizeof(float));
}
```

## 🛡️ 안전한 패치를 위한 가이드라인

### 1. 백업 및 복원 시스템
```cpp
class PatchManager {
private:
    struct PatchInfo {
        uintptr_t address;
        std::vector<BYTE> originalBytes;
        std::vector<BYTE> patchedBytes;
        bool isApplied;
    };
    
    std::vector<PatchInfo> patches;
    
public:
    bool ApplyPatch(uintptr_t address, const std::vector<BYTE>& newBytes) {
        PatchInfo patch;
        patch.address = address;
        patch.originalBytes.resize(newBytes.size());
        
        // 원본 바이트 백업
        ReadMemory(address, patch.originalBytes.data(), newBytes.size());
        
        // 패치 적용
        if (WriteMemory(address, newBytes.data(), newBytes.size())) {
            patch.patchedBytes = newBytes;
            patch.isApplied = true;
            patches.push_back(patch);
            return true;
        }
        
        return false;
    }
    
    void RestoreAll() {
        for (auto& patch : patches) {
            if (patch.isApplied) {
                WriteMemory(patch.address, patch.originalBytes.data(), patch.originalBytes.size());
                patch.isApplied = false;
            }
        }
    }
};
```

### 2. 안전 검증
```cpp
bool SafePatch(uintptr_t address, const char* expectedBytes, const char* newBytes) {
    // 1. 주소 유효성 검사
    if (IsBadReadPtr((void*)address, 1)) {
        return false;
    }
    
    // 2. 예상 바이트 확인
    if (!VerifyBytes(address, expectedBytes)) {
        return false;
    }
    
    // 3. 실행 중인 스레드 확인
    if (IsAddressInUse(address)) {
        return false;
    }
    
    // 4. 패치 적용
    return PatchMemory(address, expectedBytes, newBytes);
}
```

### 3. 오류 처리
```cpp
enum PatchResult {
    PATCH_SUCCESS,
    PATCH_INVALID_ADDRESS,
    PATCH_BYTES_MISMATCH,
    PATCH_MEMORY_PROTECTION_FAILED,
    PATCH_ADDRESS_IN_USE
};

PatchResult SafePatchWithErrorHandling(uintptr_t address, const char* expected, const char* newBytes) {
    try {
        if (IsBadReadPtr((void*)address, 1)) {
            return PATCH_INVALID_ADDRESS;
        }
        
        if (!VerifyBytes(address, expected)) {
            return PATCH_BYTES_MISMATCH;
        }
        
        // 패치 시도
        if (PatchMemory(address, expected, newBytes)) {
            return PATCH_SUCCESS;
        }
        
        return PATCH_MEMORY_PROTECTION_FAILED;
    }
    catch (...) {
        return PATCH_MEMORY_PROTECTION_FAILED;
    }
}
```

## 🔧 고급 패치 기법

### 1. 런타임 코드 생성
```cpp
#include <asmjit/asmjit.h>

void GenerateCustomCode(uintptr_t targetAddress) {
    using namespace asmjit;
    
    JitRuntime runtime;
    CodeHolder code;
    code.init(runtime.environment());
    
    x86::Assembler a(&code);
    
    // 커스텀 어셈블리 코드 생성
    a.push(x86::rax);
    a.mov(x86::rax, targetAddress);
    a.call(x86::rax);
    a.pop(x86::rax);
    a.ret();
    
    void* func;
    runtime.add(&func, &code);
    
    // 생성된 함수 사용
    ((void(*)())func)();
}
```

### 2. 조건부 패치
```cpp
void ConditionalPatch() {
    // 게임 버전 확인
    std::string version = GetGameVersion();
    
    if (version == "1.02.3") {
        ApplyPatch(0x140A2B5C1, {0x85}); // 버전 1.02.3용 패치
    }
    else if (version == "1.03.0") {
        ApplyPatch(0x140A2C1A0, {0x85}); // 버전 1.03.0용 패치  
    }
    else {
        // 자동 AOB 스캔으로 주소 찾기
        uintptr_t address = ScanForPattern("0f 84 ? ? ? ? c6");
        if (address != 0) {
            ApplyPatch(address + 1, {0x85});
        }
    }
}
```

## ⚠️ 주의사항 및 윤리적 고려사항

### 1. 법적 준수
```
✅ 허용되는 용도:
- 개인적인 학습 및 연구
- 단일 플레이어 게임의 개인적 사용
- 보안 연구 및 취약점 분석
- 오픈소스 소프트웨어 수정

❌ 금지되는 용도:
- 멀티플레이어 게임에서의 치팅
- 상업적 이익을 위한 무단 수정
- 저작권 보호 우회
- 악성 소프트웨어 개발
```

### 2. 안전 수칙
```bash
1. 항상 백업 생성
2. 오프라인 환경에서 테스트
3. 안티바이러스 예외 설정
4. 관리자 권한으로 실행
5. 게임 무결성 검사 비활성화
```

### 3. 디버깅 팁
```cpp
// 로깅 시스템
#define LOG(msg) OutputDebugStringA("[PATCH] " msg "\n")

void DebugPatch(uintptr_t address, const char* description) {
    char buffer[256];
    sprintf_s(buffer, "Patching %s at 0x%llX", description, address);
    LOG(buffer);
    
    // 패치 전 메모리 상태 기록
    BYTE beforeBytes[16];
    ReadMemory(address, beforeBytes, 16);
    
    // 패치 적용
    // ...
    
    // 패치 후 메모리 상태 기록
    BYTE afterBytes[16];
    ReadMemory(address, afterBytes, 16);
    
    // 변경사항 출력
    for (int i = 0; i < 16; i++) {
        if (beforeBytes[i] != afterBytes[i]) {
            sprintf_s(buffer, "Changed byte at +%d: 0x%02X -> 0x%02X", i, beforeBytes[i], afterBytes[i]);
            LOG(buffer);
        }
    }
}
```

## 📚 추가 학습 자료

### 권장 도서
- "Practical Reverse Engineering" - Bruce Dang
- "The IDA Pro Book" - Chris Eagle  
- "Windows Kernel Programming" - Pavel Yosifovich

### 온라인 리소스
- **Guided Hacking**: 패치 기법 튜토리얼
- **MSDN Documentation**: Windows API 레퍼런스
- **Intel Software Developer Manual**: x86-64 명령어 세트

### 실습 환경
- **Crackmes.one**: 리버싱 연습 문제
- **VulnHub**: 취약점 분석 실습
- **GitHub**: 오픈소스 게임 모딩 프로젝트

---

**💡 핵심 원칙**:
메모리 패치는 **교육 목적**과 **개인적 사용**을 위한 강력한 도구입니다. 항상 **윤리적이고 합법적인** 범위 내에서 사용하세요!