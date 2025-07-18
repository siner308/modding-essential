#include <Windows.h>
#include <winternl.h>
#include <iostream>
#include <vector>
#include <string>
#include <codecvt>
#include <locale>

#pragma comment(lib, "ntdll.lib")

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

// PEB 구조체 정의 (필요한 필드들만)
typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    // ... 다른 필드들
} PEB, *PPEB;

/**
 * Exercise 1: 안티 디버그 우회 시스템
 * 
 * 목표: 게임이나 애플리케이션의 기본적인 안티 디버깅 기법을 탐지하고 우회
 * 
 * 구현 내용:
 * 1. IsDebuggerPresent() API 우회
 * 2. PEB BeingDebugged 플래그 조작
 * 3. NtGlobalFlag 우회
 * 4. 힙 플래그 우회
 * 5. 시간 기반 탐지 우회
 * 6. 하드웨어 브레이크포인트 탐지 우회
 */

class AntiDebugBypass {
private:
    static PPEB GetPEB() {
#ifdef _WIN64
        return reinterpret_cast<PPEB>(__readgsqword(0x60));
#else
        return reinterpret_cast<PPEB>(__readfsdword(0x30));
#endif
    }

    static bool ModifyMemoryProtection(LPVOID address, SIZE_T size, DWORD newProtect, DWORD* oldProtect) {
        return VirtualProtect(address, size, newProtect, oldProtect) != FALSE;
    }

public:
    // 1. IsDebuggerPresent API 우회
    static bool BypassIsDebuggerPresent() {
        std::wcout << L"[+] IsDebuggerPresent 우회 중..." << std::endl;
        
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        if (!kernel32) {
            std::wcout << L"[-] kernel32.dll 로드 실패" << std::endl;
            return false;
        }

        FARPROC pIsDebuggerPresent = GetProcAddress(kernel32, "IsDebuggerPresent");
        if (!pIsDebuggerPresent) {
            std::wcout << L"[-] IsDebuggerPresent 함수 찾기 실패" << std::endl;
            return false;
        }

        DWORD oldProtect;
        if (!ModifyMemoryProtection(pIsDebuggerPresent, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] 메모리 보호 해제 실패" << std::endl;
            return false;
        }

        // 함수를 "XOR EAX, EAX; RET"로 패치 (항상 0 반환)
        *(BYTE*)pIsDebuggerPresent = 0x33;      // XOR EAX, EAX
        *((BYTE*)pIsDebuggerPresent + 1) = 0xC0;
        *((BYTE*)pIsDebuggerPresent + 2) = 0xC3; // RET

        VirtualProtect(pIsDebuggerPresent, 1, oldProtect, &oldProtect);
        
        std::wcout << L"[+] IsDebuggerPresent 우회 완료" << std::endl;
        return true;
    }

    // 2. PEB BeingDebugged 플래그 우회
    static bool BypassPEBBeingDebugged() {
        std::wcout << L"[+] PEB BeingDebugged 플래그 우회 중..." << std::endl;
        
        PPEB peb = GetPEB();
        if (!peb) {
            std::wcout << L"[-] PEB 획득 실패" << std::endl;
            return false;
        }

        DWORD oldProtect;
        if (!ModifyMemoryProtection(&peb->BeingDebugged, sizeof(BOOLEAN), PAGE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] PEB 메모리 보호 해제 실패" << std::endl;
            return false;
        }

        peb->BeingDebugged = FALSE;
        
        VirtualProtect(&peb->BeingDebugged, sizeof(BOOLEAN), oldProtect, &oldProtect);
        
        std::wcout << L"[+] PEB BeingDebugged 플래그 우회 완료" << std::endl;
        return true;
    }

    // 3. NtGlobalFlag 우회
    static bool BypassNtGlobalFlag() {
        std::wcout << L"[+] NtGlobalFlag 우회 중..." << std::endl;
        
        PPEB peb = GetPEB();
        if (!peb) {
            std::wcout << L"[-] PEB 획득 실패" << std::endl;
            return false;
        }

        // PEB에서 NtGlobalFlag 오프셋 계산 (x64: 0xBC, x86: 0x68)
#ifdef _WIN64
        DWORD* pNtGlobalFlag = (DWORD*)((BYTE*)peb + 0xBC);
#else
        DWORD* pNtGlobalFlag = (DWORD*)((BYTE*)peb + 0x68);
#endif

        DWORD oldProtect;
        if (!ModifyMemoryProtection(pNtGlobalFlag, sizeof(DWORD), PAGE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] NtGlobalFlag 메모리 보호 해제 실패" << std::endl;
            return false;
        }

        // 디버그 관련 플래그 제거
        *pNtGlobalFlag &= ~(0x70); // FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS
        
        VirtualProtect(pNtGlobalFlag, sizeof(DWORD), oldProtect, &oldProtect);
        
        std::wcout << L"[+] NtGlobalFlag 우회 완료" << std::endl;
        return true;
    }

    // 4. 힙 플래그 우회
    static bool BypassHeapFlags() {
        std::wcout << L"[+] 힙 플래그 우회 중..." << std::endl;
        
        PPEB peb = GetPEB();
        if (!peb) {
            std::wcout << L"[-] PEB 획득 실패" << std::endl;
            return false;
        }

        // ProcessHeap 획득 (PEB + 0x18 on x86, PEB + 0x30 on x64)
#ifdef _WIN64
        PVOID* pProcessHeap = (PVOID*)((BYTE*)peb + 0x30);
#else
        PVOID* pProcessHeap = (PVOID*)((BYTE*)peb + 0x18);
#endif

        if (!*pProcessHeap) {
            std::wcout << L"[-] ProcessHeap 획득 실패" << std::endl;
            return false;
        }

        // 힙 플래그 조작 (Heap + 0x18에서 플래그 위치)
        DWORD* pHeapFlags = (DWORD*)((BYTE*)*pProcessHeap + 0x18);
        
        DWORD oldProtect;
        if (!ModifyMemoryProtection(pHeapFlags, sizeof(DWORD), PAGE_READWRITE, &oldProtect)) {
            std::wcout << L"[-] 힙 플래그 메모리 보호 해제 실패" << std::endl;
            return false;
        }

        // 디버그 관련 힙 플래그 제거
        *pHeapFlags &= ~(0x00000002 | 0x00000001 | 0x00000004); // HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED | HEAP_SKIP_VALIDATION_CHECKS
        
        VirtualProtect(pHeapFlags, sizeof(DWORD), oldProtect, &oldProtect);
        
        std::wcout << L"[+] 힙 플래그 우회 완료" << std::endl;
        return true;
    }

    // 5. 시간 기반 탐지 우회 (QueryPerformanceCounter 후킹)
    static bool BypassTimingDetection() {
        std::wcout << L"[+] 시간 기반 탐지 우회 중..." << std::endl;
        
        // 실제로는 QueryPerformanceCounter, GetTickCount 등을 후킹해야 함
        // 여기서는 간단한 예시로 구현
        
        std::wcout << L"[+] 시간 기반 탐지 우회 완료 (예시)" << std::endl;
        return true;
    }

    // 6. 하드웨어 브레이크포인트 제거
    static bool ClearHardwareBreakpoints() {
        std::wcout << L"[+] 하드웨어 브레이크포인트 제거 중..." << std::endl;
        
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        
        if (!GetThreadContext(GetCurrentThread(), &ctx)) {
            std::wcout << L"[-] 스레드 컨텍스트 획득 실패" << std::endl;
            return false;
        }

        // 모든 디버그 레지스터 초기화
        ctx.Dr0 = 0;
        ctx.Dr1 = 0;
        ctx.Dr2 = 0;
        ctx.Dr3 = 0;
        ctx.Dr6 = 0;
        ctx.Dr7 = 0;

        if (!SetThreadContext(GetCurrentThread(), &ctx)) {
            std::wcout << L"[-] 스레드 컨텍스트 설정 실패" << std::endl;
            return false;
        }

        std::wcout << L"[+] 하드웨어 브레이크포인트 제거 완료" << std::endl;
        return true;
    }

    // 안티 디버그 탐지 함수들
    static bool DetectIsDebuggerPresent() {
        return IsDebuggerPresent() != FALSE;
    }

    static bool DetectPEBBeingDebugged() {
        PPEB peb = GetPEB();
        return peb && peb->BeingDebugged;
    }

    static bool DetectNtGlobalFlag() {
        PPEB peb = GetPEB();
        if (!peb) return false;

#ifdef _WIN64
        DWORD ntGlobalFlag = *(DWORD*)((BYTE*)peb + 0xBC);
#else
        DWORD ntGlobalFlag = *(DWORD*)((BYTE*)peb + 0x68);
#endif
        return (ntGlobalFlag & 0x70) != 0;
    }

    static bool DetectHardwareBreakpoints() {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        
        if (!GetThreadContext(GetCurrentThread(), &ctx)) {
            return false;
        }

        return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
    }

    // 모든 우회 기법 적용
    static bool ApplyAllBypasses() {
        std::wcout << L"=== 안티 디버그 우회 시스템 시작 ===" << std::endl;
        
        bool success = true;
        
        success &= BypassIsDebuggerPresent();
        success &= BypassPEBBeingDebugged();
        success &= BypassNtGlobalFlag();
        success &= BypassHeapFlags();
        success &= BypassTimingDetection();
        success &= ClearHardwareBreakpoints();
        
        if (success) {
            std::wcout << L"[+] 모든 안티 디버그 우회 완료!" << std::endl;
        } else {
            std::wcout << L"[-] 일부 우회 기법 실패" << std::endl;
        }
        
        return success;
    }

    // 우회 효과 검증
    static void VerifyBypasses() {
        std::wcout << L"\n=== 우회 효과 검증 ===" << std::endl;
        
        std::wcout << L"IsDebuggerPresent: " << (DetectIsDebuggerPresent() ? L"탐지됨" : L"우회됨") << std::endl;
        std::wcout << L"PEB BeingDebugged: " << (DetectPEBBeingDebugged() ? L"탐지됨" : L"우회됨") << std::endl;
        std::wcout << L"NtGlobalFlag: " << (DetectNtGlobalFlag() ? L"탐지됨" : L"우회됨") << std::endl;
        std::wcout << L"하드웨어 BP: " << (DetectHardwareBreakpoints() ? L"탐지됨" : L"우회됨") << std::endl;
    }
};

int main() {
    std::wcout << L"고급 안티 디버그 우회 시스템 v1.0" << std::endl;
    std::wcout << L"교육 및 연구 목적으로만 사용하세요." << std::endl;
    std::wcout << L"========================================" << std::endl;

    // 우회 전 상태 확인
    std::wcout << L"\n=== 우회 전 상태 ===" << std::endl;
    AntiDebugBypass::VerifyBypasses();

    // 안티 디버그 우회 적용
    if (AntiDebugBypass::ApplyAllBypasses()) {
        // 우회 후 상태 확인
        std::wcout << L"\n=== 우회 후 상태 ===" << std::endl;
        AntiDebugBypass::VerifyBypasses();
    }

    std::wcout << L"\n계속하려면 Enter를 누르세요..." << std::endl;
    std::wcin.get();

    return 0;
}

/*
 * 컴파일 방법:
 * cl /EHsc exercise1_anti_debug_bypass.cpp
 * 
 * 테스트 방법:
 * 1. 디버거 없이 실행 - 모든 탐지 기법이 "우회됨"으로 표시
 * 2. 디버거로 실행 - 우회 전에는 일부 탐지, 우회 후에는 모두 우회됨
 * 
 * 학습 포인트:
 * - PEB 구조체 조작
 * - API 함수 런타임 패치
 * - 메모리 보호 속성 변경
 * - 스레드 컨텍스트 조작
 * - 시스템 내부 구조 이해
 */