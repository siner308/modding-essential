/*
 * Exercise 2: 프로세스 일시정지
 * 
 * 문제: 특정 프로세스의 모든 스레드를 일시정지하는 함수를 구현하세요.
 * 
 * 학습 목표:
 * - 스레드 관리 API 사용법
 * - 프로세스와 스레드의 관계 이해
 * - 안전한 일시정지/재개 구현
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>

class ProcessController {
private:
    struct ThreadInfo {
        DWORD threadId;
        HANDLE threadHandle;
        bool wasSuspended;
    };
    
    DWORD targetProcessId;
    std::vector<ThreadInfo> threads;
    bool isPaused;

public:
    ProcessController() : targetProcessId(0), isPaused(false) {}
    
    ~ProcessController() {
        // 모든 스레드 핸들 정리
        for (auto& thread : threads) {
            if (thread.threadHandle != nullptr) {
                CloseHandle(thread.threadHandle);
            }
        }
    }
    
    bool AttachToProcess(const std::wstring& targetProcess) {
        // 프로세스 ID 찾기
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
                    targetProcessId = processEntry.th32ProcessID;
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
        
        std::wcout << L"프로세스에 연결됨: " << processName 
                   << L" (PID: " << targetProcessId << L")" << std::endl;
        
        return EnumerateThreads();
    }
    
    bool EnumerateThreads() {
        threads.clear();
        
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            std::wcout << L"스레드 스냅샷 생성 실패" << std::endl;
            return false;
        }
        
        THREADENTRY32 threadEntry;
        threadEntry.dwSize = sizeof(threadEntry);
        
        if (Thread32First(snapshot, &threadEntry)) {
            do {
                // 대상 프로세스의 스레드만 선택
                if (threadEntry.th32OwnerProcessID == targetProcessId) {
                    ThreadInfo info;
                    info.threadId = threadEntry.th32ThreadID;
                    info.wasSuspended = false;
                    
                    // 스레드 핸들 열기
                    info.threadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, 
                                                  FALSE, threadEntry.th32ThreadID);
                    
                    if (info.threadHandle != nullptr) {
                        threads.push_back(info);
                    } else {
                        std::wcout << L"스레드 핸들 열기 실패 (ID: " << threadEntry.th32ThreadID 
                                   << L", 오류: " << GetLastError() << L")" << std::endl;
                    }
                }
            } while (Thread32Next(snapshot, &threadEntry));
        }
        
        CloseHandle(snapshot);
        
        std::wcout << L"발견된 스레드 수: " << threads.size() << std::endl;
        return !threads.empty();
    }
    
    bool PauseProcess() {
        if (isPaused) {
            std::wcout << L"프로세스가 이미 일시정지 상태입니다." << std::endl;
            return true;
        }
        
        std::wcout << L"프로세스 일시정지 중..." << std::endl;
        
        int successCount = 0;
        for (auto& thread : threads) {
            if (thread.threadHandle != nullptr) {
                DWORD suspendCount = SuspendThread(thread.threadHandle);
                
                if (suspendCount != static_cast<DWORD>(-1)) {
                    thread.wasSuspended = true;
                    successCount++;
                    std::wcout << L"스레드 " << thread.threadId << L" 일시정지 완료 (중단 횟수: " 
                               << suspendCount + 1 << L")" << std::endl;
                } else {
                    std::wcout << L"스레드 " << thread.threadId << L" 일시정지 실패 (오류: " 
                               << GetLastError() << L")" << std::endl;
                }
            }
        }
        
        if (successCount > 0) {
            isPaused = true;
            std::wcout << L"프로세스 일시정지 완료 (" << successCount << L"/" << threads.size() 
                       << L" 스레드)" << std::endl;
            return true;
        } else {
            std::wcout << L"프로세스 일시정지 실패" << std::endl;
            return false;
        }
    }
    
    bool ResumeProcess() {
        if (!isPaused) {
            std::wcout << L"프로세스가 일시정지 상태가 아닙니다." << std::endl;
            return true;
        }
        
        std::wcout << L"프로세스 재개 중..." << std::endl;
        
        int successCount = 0;
        for (auto& thread : threads) {
            if (thread.threadHandle != nullptr && thread.wasSuspended) {
                DWORD suspendCount = ResumeThread(thread.threadHandle);
                
                if (suspendCount != static_cast<DWORD>(-1)) {
                    thread.wasSuspended = false;
                    successCount++;
                    std::wcout << L"스레드 " << thread.threadId << L" 재개 완료 (중단 횟수: " 
                               << suspendCount - 1 << L")" << std::endl;
                } else {
                    std::wcout << L"스레드 " << thread.threadId << L" 재개 실패 (오류: " 
                               << GetLastError() << L")" << std::endl;
                }
            }
        }
        
        if (successCount > 0) {
            isPaused = false;
            std::wcout << L"프로세스 재개 완료 (" << successCount << L"/" << threads.size() 
                       << L" 스레드)" << std::endl;
            return true;
        } else {
            std::wcout << L"프로세스 재개 실패" << std::endl;
            return false;
        }
    }
    
    void ShowThreadInfo() {
        std::wcout << L"\n=== 스레드 정보 ===" << std::endl;
        std::wcout << L"총 스레드 수: " << threads.size() << std::endl;
        std::wcout << L"프로세스 상태: " << (isPaused ? L"일시정지" : L"실행 중") << std::endl;
        
        std::wcout << L"\n스레드 목록:" << std::endl;
        for (size_t i = 0; i < threads.size(); ++i) {
            const auto& thread = threads[i];
            std::wcout << L"  " << i+1 << L". ID: " << thread.threadId;
            
            if (thread.threadHandle != nullptr) {
                // 스레드 상태 확인
                DWORD exitCode;
                if (GetExitCodeThread(thread.threadHandle, &exitCode)) {
                    if (exitCode == STILL_ACTIVE) {
                        std::wcout << L" (활성)";
                    } else {
                        std::wcout << L" (종료됨)";
                    }
                }
                
                if (thread.wasSuspended) {
                    std::wcout << L" [일시정지됨]";
                }
            }
        } else {
                std::wcout << L" (핸들 없음)";
            }
            
            std::wcout << std::endl;
        }
    }
    
    bool RefreshThreadList() {
        std::wcout << L"스레드 목록 새로고침 중..." << std::endl;
        
        // 기존 핸들들 정리
        for (auto& thread : threads) {
            if (thread.threadHandle != nullptr) {
                CloseHandle(thread.threadHandle);
            }
        }
        
        return EnumerateThreads();
    }
    
    bool IsProcessRunning() {
        HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, targetProcessId);
        if (processHandle == nullptr) {
            return false;
        }
        
        DWORD exitCode;
        bool isRunning = GetExitCodeProcess(processHandle, &exitCode) && (exitCode == STILL_ACTIVE);
        CloseHandle(processHandle);
        
        return isRunning;
    }
};

int main() {
    std::wcout << L"=== 프로세스 일시정지 도구 ===" << std::endl;
    std::wcout << L"특정 프로세스의 모든 스레드를 일시정지/재개할 수 있습니다." << std::endl;
    
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
    
    ProcessController controller;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 프로세스 이름을 입력하세요 (예: notepad.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 프로세스에 연결
    if (!controller.AttachToProcess(processName)) {
        std::wcout << L"프로세스 연결 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        // 프로세스가 여전히 실행 중인지 확인
        if (!controller.IsProcessRunning()) {
            std::wcout << L"\n프로세스가 종료되었습니다." << std::endl;
            break;
        }
        
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 프로세스 일시정지" << std::endl;
        std::wcout << L"2. 프로세스 재개" << std::endl;
        std::wcout << L"3. 스레드 정보 보기" << std::endl;
        std::wcout << L"4. 스레드 목록 새로고침" << std::endl;
        std::wcout << L"5. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1:
                controller.PauseProcess();
                break;
                
            case 2:
                controller.ResumeProcess();
                break;
                
            case 3:
                controller.ShowThreadInfo();
                break;
                
            case 4:
                controller.RefreshThreadList();
                break;
                
            case 5:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                // 종료 전에 일시정지된 프로세스가 있으면 재개
                controller.ResumeProcess();
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    // 프로그램 종료 시 자동으로 프로세스 재개
    controller.ResumeProcess();
    
    std::wcin.ignore();
    std::wcin.get();
    return 0;
}