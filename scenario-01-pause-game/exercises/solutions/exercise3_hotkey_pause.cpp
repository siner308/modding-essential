/*
 * Exercise 3: 핫키 시스템
 * 
 * 문제: F1 키를 눌렀을 때 게임을 일시정지/재개하는 시스템을 만드세요.
 * 
 * 학습 목표:
 * - 전역 핫키 시스템 구현
 * - 키보드 후킹 기법
 * - 토글 상태 관리
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

class HotkeyPauseSystem {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    std::atomic<bool> isPaused;
    std::atomic<bool> isRunning;
    std::vector<DWORD> threadIds;
    
    // 핫키 관련
    static const int HOTKEY_ID = 1;
    HWND hiddenWindow;
    std::thread hotkeyThread;

public:
    HotkeyPauseSystem() : processHandle(nullptr), processId(0), isPaused(false), 
                         isRunning(false), hiddenWindow(nullptr) {}
    
    ~HotkeyPauseSystem() {
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
        
        // 스레드 ID 수집
        if (!CollectThreadIds()) {
            std::wcout << L"스레드 수집 실패" << std::endl;
            return false;
        }
        
        // 핫키 시스템 초기화
        if (!InitializeHotkeys()) {
            std::wcout << L"핫키 시스템 초기화 실패" << std::endl;
            return false;
        }
        
        isRunning = true;
        std::wcout << L"핫키 일시정지 시스템 초기화 완료" << std::endl;
        std::wcout << L"F1 키를 눌러 게임을 일시정지/재개할 수 있습니다." << std::endl;
        return true;
    }
    
    bool InitializeHotkeys() {
        // 숨겨진 윈도우 생성 (메시지 처리용)
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = StaticWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"HotkeyPauseWindow";
        wc.cbWndExtra = sizeof(HotkeyPauseSystem*);
        
        if (!RegisterClassExW(&wc)) {
            return false;
        }
        
        hiddenWindow = CreateWindowExW(0, L"HotkeyPauseWindow", L"", 0, 
                                      0, 0, 0, 0, HWND_MESSAGE, nullptr, 
                                      GetModuleHandle(nullptr), this);
        
        if (!hiddenWindow) {
            return false;
        }
        
        // F1 키를 전역 핫키로 등록
        if (!RegisterHotKey(hiddenWindow, HOTKEY_ID, MOD_NOREPEAT, VK_F1)) {
            std::wcout << L"F1 핫키 등록 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        // 핫키 처리 스레드 시작
        hotkeyThread = std::thread(&HotkeyPauseSystem::HotkeyMessageLoop, this);
        
        return true;
    }
    
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        HotkeyPauseSystem* instance = nullptr;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            instance = reinterpret_cast<HotkeyPauseSystem*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, 0, reinterpret_cast<LONG_PTR>(instance));
        } else {
            instance = reinterpret_cast<HotkeyPauseSystem*>(GetWindowLongPtr(hwnd, 0));
        }
        
        if (instance) {
            return instance->WindowProc(hwnd, msg, wParam, lParam);
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_HOTKEY:
                if (wParam == HOTKEY_ID) {
                    TogglePause();
                }
                return 0;
                
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    void HotkeyMessageLoop() {
        MSG msg;
        while (isRunning && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    void TogglePause() {
        if (isPaused) {
            ResumeGame();
        } else {
            PauseGame();
        }
    }
    
    bool PauseGame() {
        if (isPaused) {
            return true; // 이미 일시정지 상태
        }
        
        std::wcout << L"게임 일시정지 중..." << std::endl;
        
        // 모든 스레드 일시정지
        int pausedCount = 0;
        for (DWORD threadId : threadIds) {
            HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
            if (threadHandle) {
                if (SuspendThread(threadHandle) != -1) {
                    pausedCount++;
                }
                CloseHandle(threadHandle);
            }
        }
        
        if (pausedCount > 0) {
            isPaused = true;
            std::wcout << L"게임 일시정지 완료 (" << pausedCount << L"개 스레드)" << std::endl;
            ShowPauseStatus();
            return true;
        } else {
            std::wcout << L"게임 일시정지 실패" << std::endl;
            return false;
        }
    }
    
    bool ResumeGame() {
        if (!isPaused) {
            std::wcout << L"프로세스가 일시정지 상태가 아닙니다." << std::endl;
            return true;
        }
        
        std::wcout << L"프로세스 재개 중..." << std::endl;
        
        int resumedCount = 0;
        for (DWORD threadId : threadIds) {
            HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
            if (threadHandle) {
                if (ResumeThread(threadHandle) != -1) {
                    resumedCount++;
                }
                CloseHandle(threadHandle);
            }
        }
        
        if (resumedCount > 0) {
            isPaused = false;
            std::wcout << L"게임 재개 완료 (" << resumedCount << L"개 스레드)" << std::endl;
            ShowPauseStatus();
            return true;
        } else {
            std::wcout << L"게임 재개 실패" << std::endl;
            return false;
        }
    }
    
    void ShowPauseStatus() {
        std::wcout << L"현재 상태: " << (isPaused ? L"일시정지됨" : L"실행 중") << std::endl;
        std::wcout << L"F1 키를 눌러 " << (isPaused ? L"재개" : L"일시정지") << L"하세요." << std::endl;
    }
    
    void MonitorSystem() {
        std::wcout << L"시스템 모니터링 시작..." << std::endl;
        std::wcout << L"ESC 키를 눌러 종료하세요." << std::endl;
        
        while (isRunning) {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                std::wcout << L"\n종료 신호 감지..." << std::endl;
                break;
            }
            
            // 프로세스가 여전히 실행 중인지 확인
            if (!IsProcessRunning()) {
                std::wcout << L"\n대상 프로세스가 종료되었습니다." << std::endl;
                break;
            }
            
            Sleep(100);
        }
        
        isRunning = false;
    }
    
    bool IsProcessRunning() {
        DWORD exitCode;
        if (GetExitCodeProcess(processHandle, &exitCode)) {
            return exitCode == STILL_ACTIVE;
        }
        return false;
    }
    
    void ShowAdvancedControls() {
        std::wcout << L"\n=== 고급 제어 옵션 ===" << std::endl;
        std::wcout << L"F1: 게임 일시정지/재개" << std::endl;
        std::wcout << L"F2: 강제 일시정지 (모든 스레드)" << std::endl;
        std::wcout << L"F3: 스레드 목록 새로고침" << std::endl;
        std::wcout << L"F4: 현재 상태 표시" << std::endl;
        std::wcout << L"ESC: 프로그램 종료" << std::endl;
        
        // 추가 핫키 등록
        RegisterHotKey(hiddenWindow, 2, MOD_NOREPEAT, VK_F2);
        RegisterHotKey(hiddenWindow, 3, MOD_NOREPEAT, VK_F3);
        RegisterHotKey(hiddenWindow, 4, MOD_NOREPEAT, VK_F4);
    }
    
    void HandleAdvancedHotkey(int hotkeyId) {
        switch (hotkeyId) {
            case 2: // F2 - 강제 일시정지
                ForcePause();
                break;
                
            case 3: // F3 - 스레드 새로고침
                RefreshThreadList();
                break;
                
            case 4: // F4 - 상태 표시
                ShowDetailedStatus();
                break;
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
    
    bool CollectThreadIds() {
        threadIds.clear();
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        THREADENTRY32 threadEntry;
        threadEntry.dwSize = sizeof(threadEntry);
        
        if (Thread32First(snapshot, &threadEntry)) {
            do {
                if (threadEntry.th32OwnerProcessID == processId) {
                    threadIds.push_back(threadEntry.th32ThreadID);
                }
            } while (Thread32Next(snapshot, &threadEntry));
        }
        
        CloseHandle(snapshot);
        
        std::wcout << L"발견된 스레드 수: " << threadIds.size() << std::endl;
        return !threadIds.empty();
    }
    
    void ForcePause() {
        std::wcout << L"강제 일시정지 실행..." << std::endl;
        
        // 모든 스레드를 여러 번 정지
        for (DWORD threadId : threadIds) {
            HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
            if (threadHandle) {
                for (int i = 0; i < 3; ++i) {
                    SuspendThread(threadHandle);
                }
                CloseHandle(threadHandle);
            }
        }
        
        isPaused = true;
        std::wcout << L"강제 일시정지 완료" << std::endl;
    }
    
    void RefreshThreadList() {
        std::wcout << L"스레드 목록 새로고침..." << std::endl;
        
        size_t oldCount = threadIds.size();
        CollectThreadIds();
        
        std::wcout << L"스레드 수 변경: " << oldCount << L" -> " << threadIds.size() << std::endl;
    }
    
    void ShowDetailedStatus() {
        std::wcout << L"\n=== 상세 상태 정보 ===" << std::endl;
        std::wcout << L"프로세스: " << processName << L" (PID: " << processId << L")" << std::endl;
        std::wcout << L"스레드 수: " << threadIds.size() << std::endl;
        std::wcout << L"현재 상태: " << (isPaused ? L"일시정지됨" : L"실행 중") << std::endl;
        std::wcout << L"시스템 실행 중: " << (isRunning ? L"예" : L"아니오") << std::endl;
        
        // 각 스레드의 상태 확인
        int activeThreads = 0;
        for (DWORD threadId : threadIds) {
            HANDLE threadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, threadId);
            if (threadHandle) {
                DWORD suspendCount = SuspendThread(threadHandle);
                if (suspendCount != -1) {
                    ResumeThread(threadHandle); // 원상복구
                    if (suspendCount == 0) {
                        activeThreads++;
                    }
                }
                CloseHandle(threadHandle);
            }
        }
        
        std::wcout << L"활성 스레드: " << activeThreads << L"/" << threadIds.size() << std::endl;
    }
    
    void Cleanup() {
        isRunning = false;
        
        // 게임이 일시정지된 상태라면 재개
        if (isPaused) {
            ResumeGame();
        }
        
        // 핫키 해제
        if (hiddenWindow) {
            UnregisterHotKey(hiddenWindow, HOTKEY_ID);
            UnregisterHotKey(hiddenWindow, 2);
            UnregisterHotKey(hiddenWindow, 3);
            UnregisterHotKey(hiddenWindow, 4);
            
            PostMessage(hiddenWindow, WM_QUIT, 0, 0);
        }
        
        // 스레드 종료 대기
        if (hotkeyThread.joinable()) {
            hotkeyThread.join();
        }
        
        // 윈도우 정리
        if (hiddenWindow) {
            DestroyWindow(hiddenWindow);
            hiddenWindow = nullptr;
        }
        
        // 프로세스 핸들 정리
        if (processHandle) {
            CloseHandle(processHandle);
            processHandle = nullptr;
        }
        
        std::wcout << L"핫키 일시정지 시스템 정리 완료" << std::endl;
    }
};

int main() {
    std::wcout << L"=== 핫키 게임 일시정지 시스템 ===" << std::endl;
    std::wcout << L"F1 키로 게임을 일시정지/재개할 수 있습니다." << std::endl;
    
    // 관리자 권한 확인
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            if (!elevation.TokenIsElevated) {
                std::wcout << L"경고: 관리자 권한이 필요할 수 있습니다." << std::endl;
            }
        }
        CloseHandle(hToken);
    }
    
    HotkeyPauseSystem pauseSystem;
    
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
    
    // 고급 제어 옵션 표시
    pauseSystem.ShowAdvancedControls();
    
    // 메인 루프
    pauseSystem.MonitorSystem();
    
    std::wcout << L"프로그램 종료" << std::endl;
    std::wcin.ignore();
    std::wcin.get();
    return 0;
}