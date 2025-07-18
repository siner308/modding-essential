# 🔴 고급 모딩 기법

**난이도**: 고급 | **학습 시간**: 4-6주 | **접근법**: 리버스 엔지니어링 + 보안 우회

안티 디버그, 코드 동굴, 고급 후킹 등 전문가 수준의 모딩 기법을 학습합니다.

## 📖 학습 목표

이 과정를 완료하면 다음을 할 수 있게 됩니다:

- [ ] 안티 디버그 기법 분석 및 우회하기
- [ ] 코드 동굴(Code Cave) 기법으로 새로운 기능 삽입하기
- [ ] 가상 함수 테이블(VTable) 후킹하기
- [ ] 패킹된 실행 파일 언패킹하기
- [ ] 난독화된 코드 분석 및 복원하기

## 🎯 최종 결과물

완성된 고급 모딩 시스템:
- **안티 디버그 우회 엔진** (다양한 보호 기법 무력화)
- **동적 코드 인젝션 시스템** (런타임 기능 추가)
- **메모리 보호 우회 도구** (DEP, ASLR 등)
- **자동 언패킹 시스템** (압축/암호화 해제)
- **실시간 디스어셈블리 엔진** (코드 분석 도구)

## 🛡️ 안티 디버그 기법 분석 및 우회

### 1. 일반적인 안티 디버그 기법들

```cpp
// 1. IsDebuggerPresent() 검사
bool CheckDebugger1() {
    return IsDebuggerPresent();
}

// 2. PEB (Process Environment Block) 직접 검사
bool CheckDebugger2() {
    PPEB peb = (PPEB)__readgsqword(0x60);
    return peb->BeingDebugged;
}

// 3. NtGlobalFlag 검사
bool CheckDebugger3() {
    PPEB peb = (PPEB)__readgsqword(0x60);
    return (peb->NtGlobalFlag & 0x70) != 0;
}

// 4. 힙 플래그 검사
bool CheckDebugger4() {
    PPEB peb = (PPEB)__readgsqword(0x60);
    PVOID heap = peb->ProcessHeap;
    DWORD heapFlags = *(DWORD*)((char*)heap + 0x18);
    return (heapFlags & HEAP_TAIL_CHECKING_ENABLED) || 
           (heapFlags & HEAP_FREE_CHECKING_ENABLED) ||
           (heapFlags & HEAP_SKIP_VALIDATION_CHECKS);
}

// 5. 시간 기반 탐지
bool CheckDebugger5() {
    DWORD start = GetTickCount();
    Sleep(100);
    DWORD end = GetTickCount();
    return (end - start) > 150; // 정상보다 오래 걸림
}

// 6. 하드웨어 브레이크포인트 검사
bool CheckDebugger6() {
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(GetCurrentThread(), &ctx);
    return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
}
```

### 2. 안티 디버그 우회 시스템

```cpp
// AntiDebugBypass.h
#pragma once
#include <Windows.h>
#include <winternl.h>
#include <vector>
#include <functional>

class AntiDebugBypass {
private:
    struct PatchInfo {
        uintptr_t address;
        std::vector<BYTE> originalBytes;
        std::vector<BYTE> patchBytes;
        bool isApplied;
    };
    
    static std::vector<PatchInfo> patches;
    static bool isInitialized;

public:
    static bool Initialize();
    static void Shutdown();
    
    // 기본 우회 기법들
    static bool BypassIsDebuggerPresent();
    static bool BypassPEBBeingDebugged();
    static bool BypassNtGlobalFlag();
    static bool BypassHeapFlags();
    static bool BypassOutputDebugString();
    
    // 고급 우회 기법들
    static bool HookNtQueryInformationProcess();
    static bool HookNtSetInformationThread();
    static bool BypassTiming();
    static bool BypassHardwareBreakpoints();
    
    // 동적 분석
    static std::vector<uintptr_t> FindAntiDebugCalls();
    static bool PatchAntiDebugFunction(uintptr_t address);
    
private:
    static bool ApplyPatch(uintptr_t address, const std::vector<BYTE>& newBytes);
    static bool RestorePatch(const PatchInfo& patch);
    static uintptr_t GetFunctionAddress(const std::string& module, const std::string& function);
};

// AntiDebugBypass.cpp
#include "AntiDebugBypass.h"
#include "MinHook.h"
#include <iostream>

std::vector<AntiDebugBypass::PatchInfo> AntiDebugBypass::patches;
bool AntiDebugBypass::isInitialized = false;

// 후킹할 함수들의 원본 포인터
typedef BOOL(WINAPI* IsDebuggerPresent_t)();
typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS(NTAPI* NtSetInformationThread_t)(HANDLE, THREADINFOCLASS, PVOID, ULONG);

IsDebuggerPresent_t oIsDebuggerPresent = nullptr;
NtQueryInformationProcess_t oNtQueryInformationProcess = nullptr;
NtSetInformationThread_t oNtSetInformationThread = nullptr;

bool AntiDebugBypass::Initialize() {
    if (isInitialized) return true;
    
    if (MH_Initialize() != MH_OK) {
        std::cout << "MinHook 초기화 실패" << std::endl;
        return false;
    }
    
    std::cout << "안티 디버그 우회 시스템 초기화 중..." << std::endl;
    
    // 기본 우회 기법들 적용
    BypassIsDebuggerPresent();
    BypassPEBBeingDebugged();
    BypassNtGlobalFlag();
    BypassHeapFlags();
    
    // 고급 후킹 적용
    HookNtQueryInformationProcess();
    HookNtSetInformationThread();
    
    isInitialized = true;
    std::cout << "안티 디버그 우회 완료" << std::endl;
    return true;
}

bool AntiDebugBypass::BypassIsDebuggerPresent() {
    // IsDebuggerPresent 함수를 항상 FALSE 반환하도록 후킹
    LPVOID pIsDebuggerPresent = GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsDebuggerPresent");
    
    if (!pIsDebuggerPresent) return false;
    
    if (MH_CreateHook(pIsDebuggerPresent, &hkIsDebuggerPresent, (LPVOID*)&oIsDebuggerPresent) != MH_OK) {
        return false;
    }
    
    return MH_EnableHook(pIsDebuggerPresent) == MH_OK;
}

BOOL WINAPI hkIsDebuggerPresent() {
    return FALSE; // 항상 디버거가 없다고 반환
}

bool AntiDebugBypass::BypassPEBBeingDebugged() {
    // PEB의 BeingDebugged 플래그를 0으로 설정
    PPEB peb = (PPEB)__readgsqword(0x60);
    if (!peb) return false;
    
    DWORD oldProtect;
    if (!VirtualProtect(&peb->BeingDebugged, sizeof(BYTE), PAGE_READWRITE, &oldProtect)) {
        return false;
    }
    
    peb->BeingDebugged = 0;
    
    VirtualProtect(&peb->BeingDebugged, sizeof(BYTE), oldProtect, &oldProtect);
    
    std::cout << "PEB BeingDebugged 플래그 우회 완료" << std::endl;
    return true;
}

bool AntiDebugBypass::BypassNtGlobalFlag() {
    // PEB의 NtGlobalFlag를 정상 값으로 설정
    PPEB peb = (PPEB)__readgsqword(0x60);
    if (!peb) return false;
    
    DWORD oldProtect;
    if (!VirtualProtect(&peb->NtGlobalFlag, sizeof(DWORD), PAGE_READWRITE, &oldProtect)) {
        return false;
    }
    
    peb->NtGlobalFlag &= ~0x70; // 디버그 관련 플래그 제거
    
    VirtualProtect(&peb->NtGlobalFlag, sizeof(DWORD), oldProtect, &oldProtect);
    
    std::cout << "NtGlobalFlag 우회 완료" << std::endl;
    return true;
}

bool AntiDebugBypass::HookNtQueryInformationProcess() {
    LPVOID pNtQueryInformationProcess = GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
    
    if (!pNtQueryInformationProcess) return false;
    
    if (MH_CreateHook(pNtQueryInformationProcess, &hkNtQueryInformationProcess, 
                     (LPVOID*)&oNtQueryInformationProcess) != MH_OK) {
        return false;
    }
    
    return MH_EnableHook(pNtQueryInformationProcess) == MH_OK;
}

NTSTATUS NTAPI hkNtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
                                          PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength) {
    
    NTSTATUS status = oNtQueryInformationProcess(ProcessHandle, ProcessInformationClass, 
                                                ProcessInformation, ProcessInformationLength, ReturnLength);
    
    // 특정 정보 요청을 가로채서 수정
    if (ProcessInformationClass == ProcessDebugPort || 
        ProcessInformationClass == ProcessDebugObjectHandle ||
        ProcessInformationClass == ProcessDebugFlags) {
        
        // 디버그 관련 정보 요청 시 정상 값으로 변조
        if (ProcessInformation && ProcessInformationLength >= sizeof(DWORD)) {
            *(DWORD*)ProcessInformation = 0;
        }
    }
    
    return status;
}
```

## 🕳️ 코드 동굴 (Code Cave) 기법

### 1. 코드 동굴의 개념
```
코드 동굴이란?
실행 파일 내부의 빈 공간(NULL 바이트로 채워진 영역)을 활용하여
새로운 코드를 삽입하는 기법

장점:
- 파일 크기 증가 없음
- 탐지 어려움  
- 기존 코드와 자연스럽게 통합

단점:
- 제한된 공간
- 복잡한 구현
- 디버깅 어려움
```

### 2. 코드 동굴 구현 시스템

```cpp
// CodeCave.h
#pragma once
#include <Windows.h>
#include <vector>
#include <map>

class CodeCave {
private:
    struct CaveInfo {
        uintptr_t address;      // 동굴 시작 주소
        size_t size;            // 동굴 크기
        size_t used;            // 사용된 크기
        bool isExecutable;      // 실행 가능한지
    };
    
    static std::vector<CaveInfo> availableCaves;
    static std::map<std::string, uintptr_t> allocatedFunctions;

public:
    static bool Initialize();
    static std::vector<CaveInfo> FindCodeCaves(size_t minSize = 32);
    static uintptr_t AllocateInCave(const std::vector<BYTE>& code, const std::string& name = "");
    static bool InstallTrampolineHook(uintptr_t targetAddress, uintptr_t caveFunction);
    static void ListCaves();
    
private:
    static bool IsMemoryEmpty(uintptr_t address, size_t size);
    static bool MakeExecutable(uintptr_t address, size_t size);
    static std::vector<BYTE> GenerateTrampoline(uintptr_t from, uintptr_t to);
};

// CodeCave.cpp
#include "CodeCave.h"
#include <iostream>

std::vector<CodeCave::CaveInfo> CodeCave::availableCaves;
std::map<std::string, uintptr_t> CodeCave::allocatedFunctions;

bool CodeCave::Initialize() {
    std::cout << "코드 동굴 검색 중..." << std::endl;
    
    // 메인 실행 파일 영역 검사
    HMODULE hModule = GetModuleHandle(nullptr);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    
    PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        // 실행 가능한 섹션만 검사
        if (sectionHeader[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            uintptr_t sectionBase = (uintptr_t)hModule + sectionHeader[i].VirtualAddress;
            size_t sectionSize = sectionHeader[i].Misc.VirtualSize;
            
            // 섹션 내에서 동굴 찾기
            FindCavesInSection(sectionBase, sectionSize);
        }
    }
    
    std::cout << "발견된 코드 동굴: " << availableCaves.size() << "개" << std::endl;
    return !availableCaves.empty();
}

void CodeCave::FindCavesInSection(uintptr_t sectionBase, size_t sectionSize) {
    const size_t MIN_CAVE_SIZE = 32; // 최소 동굴 크기
    
    uintptr_t currentPos = sectionBase;
    uintptr_t sectionEnd = sectionBase + sectionSize;
    
    while (currentPos < sectionEnd) {
        // 빈 공간 찾기 (연속된 0x00 또는 0xCC 바이트)
        if (*(BYTE*)currentPos == 0x00 || *(BYTE*)currentPos == 0xCC) {
            uintptr_t caveStart = currentPos;
            
            // 동굴 크기 계산
            while (currentPos < sectionEnd && 
                   (*(BYTE*)currentPos == 0x00 || *(BYTE*)currentPos == 0xCC)) {
                currentPos++;
            }
            
            size_t caveSize = currentPos - caveStart;
            
            // 최소 크기 이상인 동굴만 등록
            if (caveSize >= MIN_CAVE_SIZE) {
                CaveInfo cave;
                cave.address = caveStart;
                cave.size = caveSize;
                cave.used = 0;
                cave.isExecutable = true;
                
                availableCaves.push_back(cave);
                
                std::cout << "코드 동굴 발견: 0x" << std::hex << caveStart 
                         << " (크기: " << std::dec << caveSize << " 바이트)" << std::endl;
            }
        } else {
            currentPos++;
        }
    }
}

uintptr_t CodeCave::AllocateInCave(const std::vector<BYTE>& code, const std::string& name) {
    size_t requiredSize = code.size() + 16; // 여유 공간 포함
    
    // 적절한 크기의 동굴 찾기
    for (auto& cave : availableCaves) {
        if (cave.size - cave.used >= requiredSize) {
            uintptr_t allocAddress = cave.address + cave.used;
            
            // 메모리 보호 해제
            DWORD oldProtect;
            if (!VirtualProtect((LPVOID)allocAddress, requiredSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                continue;
            }
            
            // 코드 복사
            memcpy((void*)allocAddress, code.data(), code.size());
            
            // 메모리 보호 복원
            VirtualProtect((LPVOID)allocAddress, requiredSize, oldProtect, &oldProtect);
            
            // 사용량 업데이트
            cave.used += requiredSize;
            
            // 할당된 함수 등록
            if (!name.empty()) {
                allocatedFunctions[name] = allocAddress;
            }
            
            std::cout << "코드 동굴에 함수 할당: " << name << " (0x" << std::hex << allocAddress << ")" << std::endl;
            
            return allocAddress;
        }
    }
    
    std::cout << "적절한 코드 동굴을 찾을 수 없음 (필요 크기: " << requiredSize << ")" << std::endl;
    return 0;
}

bool CodeCave::InstallTrampolineHook(uintptr_t targetAddress, uintptr_t caveFunction) {
    // 트램폴린 코드 생성 (JMP 명령어)
    std::vector<BYTE> trampoline = GenerateTrampoline(targetAddress, caveFunction);
    
    if (trampoline.empty()) {
        return false;
    }
    
    // 원본 코드 백업
    std::vector<BYTE> originalCode(trampoline.size());
    memcpy(originalCode.data(), (void*)targetAddress, trampoline.size());
    
    // 메모리 보호 해제
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)targetAddress, trampoline.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // 트램폴린 설치
    memcpy((void*)targetAddress, trampoline.data(), trampoline.size());
    
    // 메모리 보호 복원
    VirtualProtect((LPVOID)targetAddress, trampoline.size(), oldProtect, &oldProtect);
    
    std::cout << "트램폴린 후킹 완료: 0x" << std::hex << targetAddress 
             << " → 0x" << caveFunction << std::endl;
    
    return true;
}

std::vector<BYTE> CodeCave::GenerateTrampoline(uintptr_t from, uintptr_t to) {
    std::vector<BYTE> trampoline;
    
    // 상대 점프 거리 계산
    intptr_t relativeOffset = to - (from + 5); // JMP 명령어는 5바이트
    
    // 32비트 상대 점프 범위 확인
    if (abs(relativeOffset) > 0x7FFFFFFF) {
        // 64비트 절대 점프 사용
        // MOV RAX, address (10바이트)
        trampoline.push_back(0x48); // REX.W
        trampoline.push_back(0xB8); // MOV RAX
        
        for (int i = 0; i < 8; i++) {
            trampoline.push_back((to >> (i * 8)) & 0xFF);
        }
        
        // JMP RAX (2바이트)
        trampoline.push_back(0xFF);
        trampoline.push_back(0xE0);
        
        // NOP padding
        while (trampoline.size() < 16) {
            trampoline.push_back(0x90);
        }
    } else {
        // 32비트 상대 점프 사용
        // JMP rel32 (5바이트)
        trampoline.push_back(0xE9);
        
        for (int i = 0; i < 4; i++) {
            trampoline.push_back((relativeOffset >> (i * 8)) & 0xFF);
        }
    }
    
    return trampoline;
}

// 실제 사용 예제: 새로운 기능을 게임에 삽입
void InstallCustomFunction() {
    // 삽입할 코드 (어셈블리를 기계어로 변환한 것)
    std::vector<BYTE> customCode = {
        0x50,                           // push rax
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, address
        0xFF, 0xD0,                     // call rax (커스텀 함수 호출)
        0x58,                           // pop rax
        0xC3                            // ret
    };
    
    // 커스텀 함수 주소를 코드에 삽입 (실제 구현에서는 주소 계산 필요)
    uintptr_t customFunctionAddr = (uintptr_t)MyCustomFunction;
    memcpy(&customCode[3], &customFunctionAddr, sizeof(uintptr_t));
    
    // 코드 동굴에 할당
    uintptr_t caveAddr = CodeCave::AllocateInCave(customCode, "CustomFunction");
    
    if (caveAddr) {
        // 원하는 위치에 후킹 설치
        uintptr_t targetAddr = 0x140123456; // 후킹할 주소
        CodeCave::InstallTrampolineHook(targetAddr, caveAddr);
    }
}

void MyCustomFunction() {
    // 여기에 새로운 기능 구현
    std::cout << "커스텀 함수가 실행되었습니다!" << std::endl;
}
```

## 🎣 고급 후킹 기법

### 1. VTable 후킹

```cpp
// VTableHook.h
#pragma once
#include <Windows.h>
#include <vector>
#include <map>

class VTableHook {
private:
    struct HookInfo {
        void** vtable;              // VTable 주소
        int functionIndex;          // 함수 인덱스
        void* originalFunction;     // 원본 함수
        void* hookFunction;         // 후킹 함수
        bool isInstalled;
    };
    
    static std::vector<HookInfo> hooks;

public:
    static bool HookVTableFunction(void* objectInstance, int functionIndex, void* hookFunction);
    static bool UnhookVTableFunction(void* objectInstance, int functionIndex);
    static void* GetOriginalFunction(void* objectInstance, int functionIndex);
    static void ListHookedFunctions();
    
private:
    static void** GetVTable(void* objectInstance);
    static bool IsValidVTable(void** vtable);
};

// VTableHook.cpp
#include "VTableHook.h"
#include <iostream>

std::vector<VTableHook::HookInfo> VTableHook::hooks;

bool VTableHook::HookVTableFunction(void* objectInstance, int functionIndex, void* hookFunction) {
    void** vtable = GetVTable(objectInstance);
    if (!vtable || !IsValidVTable(vtable)) {
        return false;
    }
    
    // 이미 후킹된 함수인지 확인
    for (const auto& hook : hooks) {
        if (hook.vtable == vtable && hook.functionIndex == functionIndex) {
            std::cout << "이미 후킹된 함수입니다." << std::endl;
            return false;
        }
    }
    
    // 메모리 보호 해제
    DWORD oldProtect;
    if (!VirtualProtect(&vtable[functionIndex], sizeof(void*), PAGE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // 후킹 정보 저장
    HookInfo hookInfo;
    hookInfo.vtable = vtable;
    hookInfo.functionIndex = functionIndex;
    hookInfo.originalFunction = vtable[functionIndex];
    hookInfo.hookFunction = hookFunction;
    hookInfo.isInstalled = true;
    
    // VTable 수정
    vtable[functionIndex] = hookFunction;
    
    // 메모리 보호 복원
    VirtualProtect(&vtable[functionIndex], sizeof(void*), oldProtect, &oldProtect);
    
    hooks.push_back(hookInfo);
    
    std::cout << "VTable 후킹 완료: 인덱스 " << functionIndex 
             << " (0x" << std::hex << vtable << ")" << std::endl;
    
    return true;
}

void** VTableHook::GetVTable(void* objectInstance) {
    if (!objectInstance) return nullptr;
    
    // 객체의 첫 8바이트는 VTable 포인터
    return *(void***)objectInstance;
}

bool VTableHook::IsValidVTable(void** vtable) {
    if (!vtable) return false;
    
    // VTable이 유효한 메모리 영역에 있는지 확인
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(vtable, &mbi, sizeof(mbi)) == 0) {
        return false;
    }
    
    if (mbi.State != MEM_COMMIT || !(mbi.Protect & PAGE_READONLY || mbi.Protect & PAGE_READWRITE)) {
        return false;
    }
    
    // 첫 번째 함수 포인터가 유효한지 확인
    if (VirtualQuery(vtable[0], &mbi, sizeof(mbi)) == 0) {
        return false;
    }
    
    return (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_EXECUTE_READ));
}

// 사용 예제: DirectX 11 Device VTable 후킹
class D3D11DeviceHook {
private:
    typedef HRESULT(__stdcall* CreateBuffer_t)(ID3D11Device*, const D3D11_BUFFER_DESC*, const D3D11_SUBRESOURCE_DATA*, ID3D11Buffer**);
    static CreateBuffer_t oCreateBuffer;

public:
    static bool InstallHook(ID3D11Device* device) {
        return VTableHook::HookVTableFunction(device, 5, &hkCreateBuffer); // CreateBuffer는 5번 인덱스
    }
    
    static HRESULT __stdcall hkCreateBuffer(ID3D11Device* pDevice, const D3D11_BUFFER_DESC* pDesc, 
                                          const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer) {
        std::cout << "CreateBuffer 호출됨: 크기 " << pDesc->ByteWidth << " 바이트" << std::endl;
        
        // 원본 함수 호출
        oCreateBuffer = (CreateBuffer_t)VTableHook::GetOriginalFunction(pDevice, 5);
        return oCreateBuffer(pDevice, pDesc, pInitialData, ppBuffer);
    }
};
```

### 2. IAT (Import Address Table) 후킹

```cpp
// IATHook.h
#pragma once
#include <Windows.h>
#include <string>
#include <map>

class IATHook {
private:
    struct IATHookInfo {
        std::string moduleName;
        std::string functionName;
        void* originalFunction;
        void* hookFunction;
        uintptr_t iatAddress;
    };
    
    static std::map<std::string, IATHookInfo> hooks;

public:
    static bool HookIATFunction(const std::string& moduleName, const std::string& functionName, void* hookFunction);
    static bool UnhookIATFunction(const std::string& moduleName, const std::string& functionName);
    static void* GetOriginalFunction(const std::string& moduleName, const std::string& functionName);
    
private:
    static uintptr_t FindIATEntry(const std::string& moduleName, const std::string& functionName);
    static PIMAGE_IMPORT_DESCRIPTOR GetImportDescriptor(HMODULE hModule, const std::string& moduleName);
};

// IATHook.cpp
#include "IATHook.h"
#include <iostream>

std::map<std::string, IATHook::IATHookInfo> IATHook::hooks;

bool IATHook::HookIATFunction(const std::string& moduleName, const std::string& functionName, void* hookFunction) {
    std::string key = moduleName + "::" + functionName;
    
    // 이미 후킹된 함수인지 확인
    if (hooks.find(key) != hooks.end()) {
        std::cout << "이미 후킹된 함수: " << key << std::endl;
        return false;
    }
    
    // IAT 엔트리 찾기
    uintptr_t iatAddress = FindIATEntry(moduleName, functionName);
    if (!iatAddress) {
        std::cout << "IAT 엔트리를 찾을 수 없음: " << key << std::endl;
        return false;
    }
    
    // 원본 함수 주소 저장
    void* originalFunction = *(void**)iatAddress;
    
    // 메모리 보호 해제
    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)iatAddress, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
        return false;
    }
    
    // IAT 수정
    *(void**)iatAddress = hookFunction;
    
    // 메모리 보호 복원
    VirtualProtect((LPVOID)iatAddress, sizeof(void*), oldProtect, &oldProtect);
    
    // 후킹 정보 저장
    IATHookInfo hookInfo;
    hookInfo.moduleName = moduleName;
    hookInfo.functionName = functionName;
    hookInfo.originalFunction = originalFunction;
    hookInfo.hookFunction = hookFunction;
    hookInfo.iatAddress = iatAddress;
    
    hooks[key] = hookInfo;
    
    std::cout << "IAT 후킹 완료: " << key << std::endl;
    return true;
}

uintptr_t IATHook::FindIATEntry(const std::string& moduleName, const std::string& functionName) {
    HMODULE hModule = GetModuleHandle(nullptr);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    
    PIMAGE_IMPORT_DESCRIPTOR importDesc = GetImportDescriptor(hModule, moduleName);
    if (!importDesc) return 0;
    
    // Import Name Table과 Import Address Table
    PIMAGE_THUNK_DATA nameTable = (PIMAGE_THUNK_DATA)((BYTE*)hModule + importDesc->OriginalFirstThunk);
    PIMAGE_THUNK_DATA addressTable = (PIMAGE_THUNK_DATA)((BYTE*)hModule + importDesc->FirstThunk);
    
    for (int i = 0; nameTable[i].u1.AddressOfData; i++) {
        PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hModule + nameTable[i].u1.AddressOfData);
        
        if (strcmp((char*)importByName->Name, functionName.c_str()) == 0) {
            return (uintptr_t)&addressTable[i].u1.Function;
        }
    }
    
    return 0;
}

// 사용 예제: CreateFileW 후킹
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileW_t oCreateFileW = nullptr;

HANDLE WINAPI hkCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                           LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                           DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    
    std::wcout << L"파일 열기: " << lpFileName << std::endl;
    
    oCreateFileW = (CreateFileW_t)IATHook::GetOriginalFunction("kernel32.dll", "CreateFileW");
    return oCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                       dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

void InstallFileHook() {
    IATHook::HookIATFunction("kernel32.dll", "CreateFileW", &hkCreateFileW);
}
```

## 📦 언패킹 및 난독화 해제

### 1. 일반적인 패킹 탐지

```cpp
// PackerDetector.h
#pragma once
#include <Windows.h>
#include <string>
#include <vector>

enum PackerType {
    PACKER_NONE,
    PACKER_UPX,
    PACKER_ASPACK,
    PACKER_PECOMPACT,
    PACKER_THEMIDA,
    PACKER_VMPROTECT,
    PACKER_UNKNOWN
};

class PackerDetector {
private:
    struct PackerSignature {
        PackerType type;
        std::string name;
        std::vector<BYTE> signature;
        size_t offset;
    };
    
    static std::vector<PackerSignature> signatures;

public:
    static bool Initialize();
    static PackerType DetectPacker(const std::string& filePath);
    static PackerType DetectPacker(HMODULE hModule);
    static std::string GetPackerName(PackerType type);
    static bool IsPacked(const std::string& filePath);
    
private:
    static void InitializeSignatures();
    static bool CheckSignature(const BYTE* data, size_t dataSize, const PackerSignature& sig);
    static float CalculateEntropy(const BYTE* data, size_t size);
    static bool HasSuspiciousImports(HMODULE hModule);
};

// PackerDetector.cpp
#include "PackerDetector.h"
#include <iostream>
#include <fstream>
#include <cmath>

std::vector<PackerDetector::PackerSignature> PackerDetector::signatures;

bool PackerDetector::Initialize() {
    InitializeSignatures();
    std::cout << "패커 시그니처 " << signatures.size() << "개 로드 완료" << std::endl;
    return true;
}

void PackerDetector::InitializeSignatures() {
    // UPX 시그니처
    signatures.push_back({
        PACKER_UPX,
        "UPX",
        {0x55, 0x50, 0x58, 0x21}, // "UPX!"
        0
    });
    
    // ASPack 시그니처
    signatures.push_back({
        PACKER_ASPACK,
        "ASPack",
        {0x60, 0xE8, 0x03, 0x00, 0x00, 0x00, 0xE9, 0xEB},
        0
    });
    
    // PECompact 시그니처
    signatures.push_back({
        PACKER_PECOMPACT,
        "PECompact",
        {0xEB, 0x06, 0x68, 0x00, 0x00, 0x00, 0x00, 0xC3},
        0
    });
    
    // Themida 시그니처
    signatures.push_back({
        PACKER_THEMIDA,
        "Themida",
        {0x8B, 0x85, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x85},
        0
    });
    
    // VMProtect 시그니처  
    signatures.push_back({
        PACKER_VMPROTECT,
        "VMProtect",
        {0x68, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00},
        0
    });
}

PackerType PackerDetector::DetectPacker(HMODULE hModule) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    
    // 1. 시그니처 기반 탐지
    BYTE* moduleBytes = (BYTE*)hModule;
    size_t moduleSize = ntHeaders->OptionalHeader.SizeOfImage;
    
    for (const auto& sig : signatures) {
        if (CheckSignature(moduleBytes, moduleSize, sig)) {
            std::cout << "패커 탐지: " << sig.name << std::endl;
            return sig.type;
        }
    }
    
    // 2. 휴리스틱 탐지
    PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    
    // 섹션 분석
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        // 섹션 이름 확인
        std::string sectionName = (char*)sectionHeader[i].Name;
        
        if (sectionName == "UPX0" || sectionName == "UPX1") {
            return PACKER_UPX;
        }
        if (sectionName == ".aspack" || sectionName == ".adata") {
            return PACKER_ASPACK;
        }
        if (sectionName == ".themida" || sectionName == ".winlice") {
            return PACKER_THEMIDA;
        }
        
        // 엔트로피 계산 (높은 엔트로피 = 압축/암호화)
        BYTE* sectionData = moduleBytes + sectionHeader[i].VirtualAddress;
        float entropy = CalculateEntropy(sectionData, sectionHeader[i].SizeOfRawData);
        
        if (entropy > 7.5f) { // 높은 엔트로피
            std::cout << "높은 엔트로피 섹션 발견: " << sectionName << " (엔트로피: " << entropy << ")" << std::endl;
            return PACKER_UNKNOWN;
        }
    }
    
    // 3. Import Table 분석
    if (HasSuspiciousImports(hModule)) {
        return PACKER_UNKNOWN;
    }
    
    return PACKER_NONE;
}

float PackerDetector::CalculateEntropy(const BYTE* data, size_t size) {
    if (!data || size == 0) return 0.0f;
    
    int frequency[256] = {0};
    
    // 바이트 빈도 계산
    for (size_t i = 0; i < size; i++) {
        frequency[data[i]]++;
    }
    
    // 엔트로피 계산
    float entropy = 0.0f;
    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            float probability = (float)frequency[i] / size;
            entropy -= probability * log2f(probability);
        }
    }
    
    return entropy;
}

bool PackerDetector::HasSuspiciousImports(HMODULE hModule) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    
    DWORD importRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!importRVA) return true; // Import Table이 없으면 의심스러움
    
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hModule + importRVA);
    
    int importCount = 0;
    while (importDesc->Name) {
        importCount++;
        importDesc++;
    }
    
    // Import가 너무 적으면 패킹되었을 가능성
    if (importCount < 3) {
        std::cout << "의심스러운 Import Table: " << importCount << "개 모듈만 임포트" << std::endl;
        return true;
    }
    
    return false;
}
```

### 2. 자동 언패킹 시스템

```cpp
// AutoUnpacker.h
#pragma once
#include "PackerDetector.h"
#include <functional>

class AutoUnpacker {
private:
    struct UnpackingMethod {
        PackerType type;
        std::function<bool(HMODULE)> unpackFunction;
        std::string description;
    };
    
    static std::vector<UnpackingMethod> methods;

public:
    static bool Initialize();
    static bool UnpackModule(HMODULE hModule);
    static bool DumpUnpackedModule(HMODULE hModule, const std::string& outputPath);
    
private:
    static void RegisterUnpackingMethods();
    static bool UnpackUPX(HMODULE hModule);
    static bool UnpackGeneric(HMODULE hModule);
    static bool FixImportTable(HMODULE hModule);
    static uintptr_t FindOEP(HMODULE hModule); // Original Entry Point
};

// AutoUnpacker.cpp
#include "AutoUnpacker.h"
#include <iostream>

std::vector<AutoUnpacker::UnpackingMethod> AutoUnpacker::methods;

bool AutoUnpacker::Initialize() {
    RegisterUnpackingMethods();
    std::cout << "언패킹 메소드 " << methods.size() << "개 등록 완료" << std::endl;
    return true;
}

void AutoUnpacker::RegisterUnpackingMethods() {
    methods.push_back({
        PACKER_UPX,
        UnpackUPX,
        "UPX 언패킹"
    });
    
    methods.push_back({
        PACKER_UNKNOWN,
        UnpackGeneric,
        "일반적인 언패킹"
    });
}

bool AutoUnpacker::UnpackModule(HMODULE hModule) {
    PackerType type = PackerDetector::DetectPacker(hModule);
    
    if (type == PACKER_NONE) {
        std::cout << "패킹되지 않은 모듈입니다." << std::endl;
        return true;
    }
    
    // 해당 패커에 맞는 언패킹 메소드 찾기
    for (const auto& method : methods) {
        if (method.type == type || method.type == PACKER_UNKNOWN) {
            std::cout << "언패킹 시도: " << method.description << std::endl;
            
            if (method.unpackFunction(hModule)) {
                std::cout << "언패킹 성공!" << std::endl;
                return true;
            }
        }
    }
    
    std::cout << "언패킹 실패" << std::endl;
    return false;
}

bool AutoUnpacker::UnpackUPX(HMODULE hModule) {
    // UPX 특화 언패킹 로직
    std::cout << "UPX 언패킹 시작..." << std::endl;
    
    // UPX의 특성상 실행 후 자동으로 언패킹됨
    // 여기서는 언패킹된 상태를 탐지하고 덤프
    
    // OEP 찾기
    uintptr_t oep = FindOEP(hModule);
    if (!oep) {
        return false;
    }
    
    std::cout << "OEP 발견: 0x" << std::hex << oep << std::endl;
    
    // Import Table 복구
    return FixImportTable(hModule);
}

bool AutoUnpacker::UnpackGeneric(HMODULE hModule) {
    std::cout << "일반적인 언패킹 방법 시도..." << std::endl;
    
    // 1. 메모리 덤프
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    
    // 2. 섹션별 특성 분석
    PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        BYTE* sectionData = (BYTE*)hModule + sectionHeader[i].VirtualAddress;
        
        // 엔트로피가 낮아진 섹션 찾기 (언패킹됨)
        float entropy = PackerDetector::CalculateEntropy(sectionData, sectionHeader[i].SizeOfRawData);
        
        if (entropy < 6.0f && (sectionHeader[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
            std::cout << "언패킹된 코드 섹션 발견: " << (char*)sectionHeader[i].Name << std::endl;
            return true;
        }
    }
    
    return false;
}

uintptr_t AutoUnpacker::FindOEP(HMODULE hModule) {
    // 다양한 방법으로 OEP 찾기
    
    // 1. 스택 기반 탐지
    // 2. API 호출 패턴 분석
    // 3. 코드 시그니처 매칭
    
    // 간단한 구현: EntryPoint 주변 검사
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    
    uintptr_t entryPoint = (uintptr_t)hModule + ntHeaders->OptionalHeader.AddressOfEntryPoint;
    
    // EntryPoint 주변에서 실제 OEP 패턴 찾기
    for (int offset = -1000; offset <= 1000; offset += 4) {
        uintptr_t checkAddr = entryPoint + offset;
        
        // 일반적인 프로그램 시작 패턴 확인
        if (*(WORD*)checkAddr == 0x5548) { // PUSH EBP; MOV EBP, ESP
            return checkAddr;
        }
    }
    
    return entryPoint; // 기본값 반환
}
```

## 🔬 실습 과제

### 과제 1: 안티 디버그 우회 도구 (고급)
- [ ] **IsDebuggerPresent 우회**: 다양한 탐지 방법 무력화
- [ ] **PEB 조작**: BeingDebugged, NtGlobalFlag 수정
- [ ] **타이밍 공격 방어**: Sleep 패턴 정규화
- [ ] **하드웨어 BP 숨김**: Debug Register 조작

### 과제 2: 코드 동굴 익스플로잇 (전문가)
- [ ] **자동 동굴 탐지**: 실행 파일 내 빈 공간 스캔
- [ ] **트램폴린 생성기**: 자동 후킹 코드 생성
- [ ] **기능 인젝션**: 새로운 게임 기능 삽입
- [ ] **스텔스 모드**: 탐지 불가능한 코드 은닉

### 과제 3: 패커 분석 시스템 (전문가)
- [ ] **패커 식별**: 시그니처 + 휴리스틱 탐지
- [ ] **자동 언패킹**: 런타임 메모리 덤프
- [ ] **Import 복구**: IAT 재구성 시스템
- [ ] **OEP 탐지**: 원본 진입점 자동 발견

## ⚠️ 고급 보안 고려사항

### 1. 커널 레벨 보호 우회
```cpp
// 커널 패치 가드 (Kernel Patch Protection) 우회
class KPPBypass {
public:
    static bool DisableKPP() {
        // 매우 위험한 작업 - 시스템 불안정 가능
        // 실제 구현 시 신중한 접근 필요
        return false; // 안전상 비활성화
    }
};
```

### 2. 하이퍼바이저 탐지 및 우회
```cpp
// VM 환경 탐지
class VMDetector {
public:
    static bool IsRunningInVM() {
        // CPUID 명령어로 하이퍼바이저 비트 확인
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        return (cpuInfo[2] & (1 << 31)) != 0;
    }
    
    static bool DetectVMware() {
        __try {
            __asm {
                mov eax, 'VMXh'
                mov ebx, 0
                mov ecx, 10
                mov edx, 'VX'
                in eax, dx
            }
            return true;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
};
```

### 3. 코드 무결성 검증 우회
```cpp
// 자체 체크섬 우회
class IntegrityBypass {
private:
    static std::map<uintptr_t, DWORD> originalChecksums;

public:
    static void SaveOriginalChecksum(uintptr_t address, DWORD checksum) {
        originalChecksums[address] = checksum;
    }
    
    static DWORD GetFakeChecksum(uintptr_t address) {
        auto it = originalChecksums.find(address);
        return (it != originalChecksums.end()) ? it->second : 0;
    }
    
    static bool HookChecksumFunction(uintptr_t functionAddr) {
        // 체크섬 함수를 가짜 결과 반환하도록 후킹
        return true;
    }
};
```

## 💡 윤리적 고려사항

### 안전한 사용을 위한 가이드라인

```cpp
class EthicalGuidelines {
public:
    static bool IsEthicalUse(const std::string& purpose) {
        // 허용되는 용도
        std::vector<std::string> allowedPurposes = {
            "보안 연구",
            "교육 목적",
            "개인적 학습",
            "단일 플레이어 게임 모딩",
            "취약점 분석"
        };
        
        for (const auto& allowed : allowedPurposes) {
            if (purpose.find(allowed) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    static void ShowWarning() {
        std::cout << "⚠️ 경고: 이 기법들은 교육 목적으로만 사용해야 합니다." << std::endl;
        std::cout << "불법적인 용도로 사용 시 법적 책임이 따를 수 있습니다." << std::endl;
        std::cout << "온라인 게임에서의 치팅은 절대 금지입니다." << std::endl;
    }
};
```

## 📊 성능 및 안정성

### 1. 안전한 실행 환경
```cpp
class SafetyManager {
private:
    static bool isTestMode;
    static std::vector<std::function<void()>> rollbackFunctions;

public:
    static void EnableTestMode() {
        isTestMode = true;
        std::cout << "테스트 모드 활성화 - 모든 변경사항은 임시적입니다." << std::endl;
    }
    
    static void RegisterRollback(std::function<void()> rollbackFunc) {
        rollbackFunctions.push_back(rollbackFunc);
    }
    
    static void RollbackAll() {
        for (auto& func : rollbackFunctions) {
            func();
        }
        rollbackFunctions.clear();
        std::cout << "모든 변경사항이 복원되었습니다." << std::endl;
    }
};
```

## 📚 추가 학습 자료

- [Practical Reverse Engineering](https://www.wiley.com/en-us/Practical+Reverse+Engineering%3A+x86%2C+x64%2C+ARM%2C+Windows+Kernel%2C+Reversing+Tools%2C+and+Obfuscation-p-9781118787311) - 실용적인 리버스 엔지니어링: x86, x64, ARM, Windows 커널, 리버싱 도구 및 난독화
- [Rootkits and Bootkits](https://nostarch.com/rootkits) - 루트킷 및 부트킷: 고급 시스템 해킹 및 방어
- [The Art of Memory Forensics](https://www.wiley.com/en-us/The+Art+of+Memory+Forensics%3A+Detecting+Malware+and+Threats+in+Windows%2C+Linux%2C+and+Mac+Memory-p-9781118825099) - 메모리 포렌식의 기술: Windows, Linux, Mac 메모리에서 악성코드 및 위협 탐지

---

**⚡ 완료 예상 시간**: 28-42일 (하루 2-3시간 투자 기준)

**🎓 이 과정를 완료하면 전문가 수준의 리버스 엔지니어링 및 모딩 기술을 보유하게 됩니다!**

**다음 단계**: [Projects](../projects/) 폴더에서 실전 프로젝트에 도전하거나, 커뮤니티에 참여하여 지식을 공유하세요.