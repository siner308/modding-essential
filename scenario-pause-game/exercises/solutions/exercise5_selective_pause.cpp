/*
 * Exercise 5: 고급 일시정지
 * 
 * 문제: 특정 기능만 일시정지하고 UI는 동작하도록 하는 선택적 일시정지를 구현하세요.
 * 
 * 학습 목표:
 * - 스레드 분석 및 분류
 * - 선택적 스레드 제어
 * - 게임 구조 이해
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <dbghelp.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <thread>
#include <atomic>

#pragma comment(lib, "dbghelp.lib")

class SelectivePauseSystem {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    std::atomic<bool> isRunning;
    
    enum class ThreadType {
        Unknown,
        MainGameplay,
        Rendering,
        Audio,
        Input,
        Network,
        UI,
        Physics,
        AI,
        Animation,
        Loading
    };
    
    struct ThreadInfo {
        DWORD threadId;
        HANDLE threadHandle;
        ThreadType type;
        std::string description;
        bool isPaused;
        int suspendCount;
        uintptr_t startAddress;
        std::string moduleName;
        DWORD cpuUsage;
        bool isEssential; // UI, 입력 등 계속 실행되어야 하는 스레드
    };
    
    std::map<DWORD, ThreadInfo> threads;
    std::vector<ThreadType> pausedTypes;

public:
    SelectivePauseSystem() : processHandle(nullptr), processId(0), isRunning(false) {}
    
    ~SelectivePauseSystem() {
        Cleanup();
    }
    
    bool Initialize(const std::wstring& targetProcess) {
        processName = targetProcess;
        
        // 프로세스 찾기
        if (!FindProcess()) {
            return false;
        }
        
        // 프로세스 핸들 열기
        processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        // 디버그 심볼 초기화
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        if (!SymInitialize(processHandle, nullptr, TRUE)) {
            std::wcout << L"심볼 초기화 실패. 기본 분석 모드로 실행합니다." << std::endl;
        }
        
        // 스레드 분석
        if (!AnalyzeThreads()) {
            std::wcout << L"스레드 분석 실패" << std::endl;
            return false;
        }
        
        isRunning = true;
        std::wcout << L"선택적 일시정지 시스템 초기화 완료" << std::endl;
        return true;
    }
    
    bool AnalyzeThreads() {
        threads.clear();
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        THREADENTRY32 threadEntry;
        threadEntry.dwSize = sizeof(threadEntry);
        
        if (Thread32First(snapshot, &threadEntry)) {
            do {
                if (threadEntry.th32OwnerProcessID == processId) {
                    ThreadInfo info = {};
                    info.threadId = threadEntry.th32ThreadID;
                    info.threadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
                    info.isPaused = false;
                    info.suspendCount = 0;
                    
                    if (info.threadHandle) {
                        // 스레드 시작 주소 가져오기
                        info.startAddress = GetThreadStartAddress(info.threadHandle);
                        
                        // 모듈 이름 확인
                        info.moduleName = GetModuleName(info.startAddress);
                        
                        // 스레드 타입 분류
                        info.type = ClassifyThread(info);
                        info.description = GetThreadDescription(info);
                        info.isEssential = IsEssentialThread(info.type);
                        
                        threads[info.threadId] = info;
                    }
                }
            } while (Thread32Next(snapshot, &threadEntry));
        }
        
        CloseHandle(snapshot);
        
        std::wcout << L"분석된 스레드 수: " << threads.size() << std::endl;
        return !threads.empty();
    }
    
    uintptr_t GetThreadStartAddress(HANDLE threadHandle) {
        NTSTATUS (WINAPI *pNtQueryInformationThread)(HANDLE, ULONG, PVOID, ULONG, PULONG);
        pNtQueryInformationThread = reinterpret_cast<decltype(pNtQueryInformationThread)>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread"));
        
        if (!pNtQueryInformationThread) {
            return 0;
        }
        
        uintptr_t startAddress = 0;
        ULONG returnLength;
        
        // ThreadQuerySetWin32StartAddress = 9
        NTSTATUS status = pNtQueryInformationThread(threadHandle, 9, 
                                                   &startAddress, sizeof(startAddress), &returnLength);
        
        return (status == 0) ? startAddress : 0;
    }
    
    std::string GetModuleName(uintptr_t address) {
        if (address == 0) {
            return "Unknown";
        }
        
        HMODULE hModules[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(processHandle, hModules, sizeof(hModules), &cbNeeded)) {
            DWORD moduleCount = cbNeeded / sizeof(HMODULE);
            
            for (DWORD i = 0; i < moduleCount; ++i) {
                MODULEINFO modInfo;
                if (GetModuleInformation(processHandle, hModules[i], &modInfo, sizeof(modInfo))) {
                    uintptr_t moduleStart = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                    uintptr_t moduleEnd = moduleStart + modInfo.SizeOfImage;
                    
                    if (address >= moduleStart && address < moduleEnd) {
                        char moduleName[MAX_PATH];
                        if (GetModuleBaseNameA(processHandle, hModules[i], moduleName, sizeof(moduleName))) {
                            return std::string(moduleName);
                        }
                    }
                }
            }
        }
        
        return "Unknown";
    }
    
    ThreadType ClassifyThread(const ThreadInfo& info) {
        std::string lowerModuleName = info.moduleName;
        std::transform(lowerModuleName.begin(), lowerModuleName.end(), 
                      lowerModuleName.begin(), ::tolower);
        
        // 모듈 이름 기반 분류
        if (lowerModuleName.find("d3d") != std::string::npos || 
            lowerModuleName.find("opengl") != std::string::npos ||
            lowerModuleName.find("vulkan") != std::string::npos ||
            lowerModuleName.find("graphics") != std::string::npos) {
            return ThreadType::Rendering;
        }
        
        if (lowerModuleName.find("audio") != std::string::npos ||
            lowerModuleName.find("sound") != std::string::npos ||
            lowerModuleName.find("wasapi") != std::string::npos ||
            lowerModuleName.find("dsound") != std::string::npos) {
            return ThreadType::Audio;
        }
        
        if (lowerModuleName.find("input") != std::string::npos ||
            lowerModuleName.find("keyboard") != std::string::npos ||
            lowerModuleName.find("mouse") != std::string::npos ||
            lowerModuleName.find("xinput") != std::string::npos) {
            return ThreadType::Input;
        }
        
        if (lowerModuleName.find("network") != std::string::npos ||
            lowerModuleName.find("winsock") != std::string::npos ||
            lowerModuleName.find("ws2_32") != std::string::npos) {
            return ThreadType::Network;
        }
        
        if (lowerModuleName.find("ui") != std::string::npos ||
            lowerModuleName.find("gui") != std::string::npos ||
            lowerModuleName.find("user32") != std::string::npos) {
            return ThreadType::UI;
        }
        
        // 스레드 시작 주소 기반 추가 분류
        if (info.startAddress != 0) {
            // 메인 실행 파일에서 시작된 스레드 중 첫 번째는 보통 메인 게임플레이
            static bool mainGameplayFound = false;
            if (!mainGameplayFound && lowerModuleName == "eldenring.exe") {
                mainGameplayFound = true;
                return ThreadType::MainGameplay;
            }
        }
        
        // 기본값
        return ThreadType::Unknown;
    }
    
    std::string GetThreadDescription(const ThreadInfo& info) {
        switch (info.type) {
            case ThreadType::MainGameplay:
                return "게임 로직 (적, 플레이어, 게임 규칙)";
            case ThreadType::Rendering:
                return "렌더링 (그래픽, 화면 출력)";
            case ThreadType::Audio:
                return "오디오 (음악, 효과음)";
            case ThreadType::Input:
                return "입력 처리 (키보드, 마우스, 컨트롤러)";
            case ThreadType::Network:
                return "네트워크 (온라인 기능)";
            case ThreadType::UI:
                return "사용자 인터페이스 (메뉴, HUD)";
            case ThreadType::Physics:
                return "물리 연산 (충돌, 중력)";
            case ThreadType::AI:
                return "인공지능 (NPC 행동)";
            case ThreadType::Animation:
                return "애니메이션 (캐릭터 움직임)";
            case ThreadType::Loading:
                return "로딩 (데이터 불러오기)";
            default:
                return "알 수 없는 기능 (" + info.moduleName + ")";
        }
    }
    
    bool IsEssentialThread(ThreadType type) {
        switch (type) {
            case ThreadType::UI:
            case ThreadType::Input:
            case ThreadType::Audio:  // 오디오는 선택적으로 유지
                return true;
            default:
                return false;
        }
    }
    
    void ShowThreadAnalysis() {
        std::wcout << L"\n=== 스레드 분석 결과 ===" << std::endl;
        
        std::map<ThreadType, int> typeCounts;
        for (const auto& pair : threads) {
            typeCounts[pair.second.type]++;
        }
        
        std::wcout << L"스레드 타입별 분포:" << std::endl;
        for (const auto& pair : typeCounts) {
            std::wcout << L"  " << GetThreadTypeName(pair.first) << L": " << pair.second << L"개" << std::endl;
        }
        
        std::wcout << L"\n상세 스레드 목록:" << std::endl;
        int index = 1;
        for (const auto& pair : threads) {
            const auto& info = pair.second;
            std::wcout << L"  " << index++ << L". [" << GetThreadTypeName(info.type) << L"] "
                       << std::wstring(info.description.begin(), info.description.end())
                       << L" (TID: " << info.threadId << L")"
                       << (info.isEssential ? L" [필수]" : L"")
                       << (info.isPaused ? L" [일시정지됨]" : L"") << std::endl;
        }
    }
    
    std::wstring GetThreadTypeName(ThreadType type) {
        switch (type) {
            case ThreadType::MainGameplay: return L"게임로직";
            case ThreadType::Rendering: return L"렌더링";
            case ThreadType::Audio: return L"오디오";
            case ThreadType::Input: return L"입력";
            case ThreadType::Network: return L"네트워크";
            case ThreadType::UI: return L"UI";
            case ThreadType::Physics: return L"물리";
            case ThreadType::AI: return L"AI";
            case ThreadType::Animation: return L"애니메이션";
            case ThreadType::Loading: return L"로딩";
            default: return L"알수없음";
        }
    }
    
    bool PauseThreadType(ThreadType type, bool excludeEssential = true) {
        if (std::find(pausedTypes.begin(), pausedTypes.end(), type) != pausedTypes.end()) {
            std::wcout << L"이미 일시정지된 타입입니다: " << GetThreadTypeName(type) << std::endl;
            return true;
        }
        
        int pausedCount = 0;
        for (auto& pair : threads) {
            auto& info = pair.second;
            
            if (info.type == type && (!excludeEssential || !info.isEssential) && !info.isPaused) {
                if (SuspendThread(info.threadHandle) != -1) {
                    info.isPaused = true;
                    info.suspendCount++;
                    pausedCount++;
                }
            }
        }
        
        if (pausedCount > 0) {
            pausedTypes.push_back(type);
            std::wcout << GetThreadTypeName(type) << L" 스레드 " << pausedCount << L"개 일시정지됨" << std::endl;
            return true;
        } else {
            std::wcout << L"일시정지할 " << GetThreadTypeName(type) << L" 스레드가 없습니다" << std::endl;
            return false;
        }
    }
    
    bool ResumeThreadType(ThreadType type) {
        auto it = std::find(pausedTypes.begin(), pausedTypes.end(), type);
        if (it == pausedTypes.end()) {
            std::wcout << L"일시정지되지 않은 타입입니다: " << GetThreadTypeName(type) << std::endl;
            return true;
        }
        
        int resumedCount = 0;
        for (auto& pair : threads) {
            auto& info = pair.second;
            
            if (info.type == type && info.isPaused) {
                if (ResumeThread(info.threadHandle) != -1) {
                    info.isPaused = false;
                    info.suspendCount = 0;
                    resumedCount++;
                }
            }
        }
        
        if (resumedCount > 0) {
            pausedTypes.erase(it);
            std::wcout << GetThreadTypeName(type) << L" 스레드 " << resumedCount << L"개 재개됨" << std::endl;
            return true;
        } else {
            std::wcout << L"재개할 " << GetThreadTypeName(type) << L" 스레드가 없습니다" << std::endl;
            return false;
        }
    }
    
    void ShowPausePresets() {
        std::wcout << L"\n=== 일시정지 프리셋 ===" << std::endl;
        std::wcout << L"1. 게임플레이만 정지 (UI와 오디오 유지)" << std::endl;
        std::wcout << L"2. 게임플레이 + 물리 정지" << std::endl;
        std::wcout << L"3. 렌더링 제외 모든 것 정지" << std::endl;
        std::wcout << L"4. 네트워크만 정지" << std::endl;
        std::wcout << L"5. 커스텀 선택" << std::endl;
    }
    
    void ApplyPausePreset(int preset) {
        switch (preset) {
            case 1: // 게임플레이만 정지
                PauseThreadType(ThreadType::MainGameplay);
                PauseThreadType(ThreadType::Physics);
                PauseThreadType(ThreadType::AI);
                std::wcout << L"게임플레이 일시정지 적용 (UI, 오디오, 입력 유지)" << std::endl;
                break;
                
            case 2: // 게임플레이 + 물리 정지
                PauseThreadType(ThreadType::MainGameplay);
                PauseThreadType(ThreadType::Physics);
                PauseThreadType(ThreadType::AI);
                PauseThreadType(ThreadType::Animation);
                std::wcout << L"게임플레이 + 물리 일시정지 적용" << std::endl;
                break;
                
            case 3: // 렌더링 제외 모든 것 정지
                PauseThreadType(ThreadType::MainGameplay);
                PauseThreadType(ThreadType::Physics);
                PauseThreadType(ThreadType::AI);
                PauseThreadType(ThreadType::Animation);
                PauseThreadType(ThreadType::Network);
                std::wcout << L"렌더링 제외 일시정지 적용" << std::endl;
                break;
                
            case 4: // 네트워크만 정지
                PauseThreadType(ThreadType::Network);
                std::wcout << L"네트워크 일시정지 적용" << std::endl;
                break;
                
            case 5: // 커스텀 선택
                ShowCustomPauseMenu();
                break;
                                
            default:
                std::wcout << L"잘못된 프리셋 번호입니다" << std::endl;
                break;
        }
    }
    
    void ShowCustomPauseMenu() {
        std::wcout << L"\n=== 커스텀 일시정지 선택 ===" << std::endl;
        std::wcout << L"일시정지할 스레드 타입을 선택하세요 (여러 개 가능, 0으로 완료):" << std::endl;
        std::wcout << L"1. 게임 로직" << std::endl;
        std::wcout << L"2. 렌더링" << std::endl;
        std::wcout << L"3. 오디오" << std::endl;
        std::wcout << L"4. 입력" << std::endl;
        std::wcout << L"5. 네트워크" << std::endl;
        std::wcout << L"6. 물리" << std::endl;
        std::wcout << L"7. AI" << std::endl;
        std::wcout << L"8. 애니메이션" << std::endl;
        
        std::vector<ThreadType> selectedTypes;
        int choice;
        
        while (true) {
            std::wcout << L"선택 (0=완료): ";
            std::wcin >> choice;
            
            if (choice == 0) break;
            
            ThreadType type;
            switch (choice) {
                case 1: type = ThreadType::MainGameplay; break;
                case 2: type = ThreadType::Rendering; break;
                case 3: type = ThreadType::Audio; break;
                case 4: type = ThreadType::Input; break;
                case 5: type = ThreadType::Network; break;
                case 6: type = ThreadType::Physics; break;
                case 7: type = ThreadType::AI; break;
                case 8: type = ThreadType::Animation; break;
                default:
                    std::wcout << L"잘못된 선택입니다" << std::endl;
                    continue;
            }
            
            selectedTypes.push_back(type);
            std::wcout << GetThreadTypeName(type) << L" 추가됨" << std::endl;
        }
        
        std::wcout << L"\n선택된 타입들을 일시정지합니다..." << std::endl;
        for (ThreadType type : selectedTypes) {
            PauseThreadType(type);
        }
    }
    
    void ResumeAll() {
        std::wcout << L"모든 일시정지된 스레드를 재개합니다..." << std::endl;
        
        auto pausedTypesCopy = pausedTypes; // 복사본 생성 (반복 중 수정 방지)
        for (ThreadType type : pausedTypesCopy) {
            ResumeThreadType(type);
        }
        
        std::wcout << L"모든 스레드 재개 완료" << std::endl;
    }
    
    void ShowCurrentStatus() {
        std::wcout << L"\n=== 현재 일시정지 상태 ===" << std::endl;
        
        if (pausedTypes.empty()) {
            std::wcout << L"일시정지된 스레드 타입이 없습니다" << std::endl;
            return;
        }
        
        std::wcout << L"일시정지된 타입:" << std::endl;
        for (ThreadType type : pausedTypes) {
            int count = 0;
            for (const auto& pair : threads) {
                if (pair.second.type == type && pair.second.isPaused) {
                    count++;
                }
            }
            std::wcout << L"  " << GetThreadTypeName(type) << L": " << count << L"개 스레드" << std::endl;
        }
    }
    
private:
    bool FindProcess() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            std::wcout << L"프로세스 스냅샷 생성 실패" << std::endl;
            return false;
        }
        
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        bool found = false;
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    processId = processEntry.th32ProcessID;
                    found = true;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        
        CloseHandle(snapshot);
        
        if (!found) {
            std::wcout << L"프로세스를 찾을 수 없습니다: " << processName << std::endl;
            return false;
        }
        
        std::wcout << L"프로세스 발견: " << processName << L" (PID: " << processId << L")" << std::endl;
        return true;
    }
    
    void Cleanup() {
        // 모든 일시정지 해제
        ResumeAll();
        
        // 스레드 핸들 정리
        for (auto& pair : threads) {
            if (pair.second.threadHandle) {
                CloseHandle(pair.second.threadHandle);
            }
        }
        
        // 심볼 정리
        if (processHandle) {
            SymCleanup(processHandle);
            CloseHandle(processHandle);
        }
        
        isRunning = false;
    }
};

int main() {
    std::wcout << L"=== 선택적 게임 일시정지 시스템 ===" << std::endl;
    std::wcout << L"게임의 특정 기능만 일시정지하여 UI와 입력은 유지합니다." << std::endl;
    
    SelectivePauseSystem pauseSystem;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 시스템 초기화
    if (!pauseSystem.Initialize(processName)) {
        std::wcout << L"시스템 초기화 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 스레드 분석 결과 보기" << std::endl;
        std::wcout << L"2. 일시정지 프리셋 적용" << std::endl;
        std::wcout << L"3. 현재 상태 보기" << std::endl;
        std::wcout << L"4. 모든 스레드 재개" << std::endl;
        std::wcout << L"5. 스레드 분석 새로고침" << std::endl;
        std::wcout << L"6. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1:
                pauseSystem.ShowThreadAnalysis();
                break;
                
            case 2:
                pauseSystem.ShowPausePresets();
                std::wcout << L"프리셋 선택: ";
                int preset;
                std::wcin >> preset;
                pauseSystem.ApplyPausePreset(preset);
                break;
                
            case 3:
                pauseSystem.ShowCurrentStatus();
                break;
                
            case 4:
                pauseSystem.ResumeAll();
                break;
                
            case 5:
                std::wcout << L"스레드 분석을 새로고침합니다..." << std::endl;
                pauseSystem.AnalyzeThreads();
                std::wcout << L"새로고침 완료" << std::endl;
                break;
                
            case 6:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}