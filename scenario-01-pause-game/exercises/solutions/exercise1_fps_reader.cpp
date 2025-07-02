/*
 * Exercise 1: 기본 메모리 스캔 - FPS 값 읽기
 * 
 * 문제: 게임의 현재 FPS 값을 찾아 읽어오는 프로그램을 작성하세요.
 * 
 * 학습 목표:
 * - 프로세스 메모리 접근 방법
 * - 메모리 스캔 기초
 * - Windows API 활용
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath> // For std::isfinite

class FPSReader {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;

public:
    FPSReader() : processHandle(nullptr), processId(0) {}
    
    ~FPSReader() {
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }
    
    bool AttachToProcess(const std::wstring& targetProcess) {
        processName = targetProcess;
        
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
        
        // 프로세스 핸들 열기
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        std::wcout << L"프로세스에 성공적으로 연결됨: " << processName 
                   << L" (PID: " << processId << L")" << std::endl;
        return true;
    }
    
    std::vector<uintptr_t> ScanForFPSValues() {
        std::vector<uintptr_t> foundAddresses;
        
        // 일반적인 FPS 값들 (30, 60, 120, 144 등)
        std::vector<float> commonFPS = {30.0f, 60.0f, 90.0f, 120.0f, 144.0f, 165.0f, 240.0f};
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
        
        std::wcout << L"메모리 스캔 시작..." << std::endl;
        
        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }
            
            // 읽기 가능한 메모리 영역만 스캔
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || 
                 mbi.Protect == PAGE_READONLY ||
                 mbi.Protect == PAGE_EXECUTE_READ ||
                 mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                auto addresses = ScanMemoryRegion(currentAddress, mbi.RegionSize, commonFPS);
                foundAddresses.insert(foundAddresses.end(), addresses.begin(), addresses.end());
            }
            
            currentAddress += mbi.RegionSize;
            
            // 진행률 표시
            if (currentAddress % (100 * 1024 * 1024) == 0) { // 100MB마다
                float progress = static_cast<float>(currentAddress) / maxAddress * 100.0f;
                std::wcout << L"\r진행률: " << std::fixed << std::setprecision(1) << progress << L"%";
            }
        }
        
        std::wcout << std::endl << L"스캔 완료. " << foundAddresses.size() << L"개의 주소 발견" << std::endl;
        return foundAddresses;
    }
    
    bool ReadFPSValue(uintptr_t address, float& fpsValue) {
        SIZE_T bytesRead;
        return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), 
                               &fpsValue, sizeof(float), &bytesRead) && bytesRead == sizeof(float);
    }
    
    void MonitorFPS(const std::vector<uintptr_t>& addresses, int duration = 10) {
        if (addresses.empty()) {
            std::wcout << L"모니터링할 주소가 없습니다." << std::endl;
            return;
        }
        
        std::wcout << L"FPS 모니터링 시작 (" << duration << L"초)..." << std::endl;
        std::wcout << L"ESC 키를 눌러 종료하세요." << std::endl;
        
        auto startTime = GetTickCount64();
        auto endTime = startTime + (duration * 1000);
        
        while (GetTickCount64() < endTime) {
            // ESC 키 확인
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                std::wcout << L"\n사용자에 의해 모니터링 중단됨" << std::endl;
                break;
            }
            
            std::wcout << L"\r시각: " << GetTickCount64() << L" | ";
            
            // 각 주소의 FPS 값 읽기
            for (size_t i = 0; i < std::min<size_t>(addresses.size(), 5); ++i) {
                float fps;
                if (ReadFPSValue(addresses[i], fps)) {
                    std::wcout << L"주소" << i+1 << L": " << std::fixed << std::setprecision(1) << fps << L" | ";
                } else {
                    std::wcout << L"주소" << i+1 << L": 오류 | ";
                }
            }
            
            Sleep(100); // 0.1초 간격
        }
        
        std::wcout << std::endl << L"모니터링 완료" << std::endl;
    }
    
private:
    std::vector<uintptr_t> ScanMemoryRegion(uintptr_t baseAddress, SIZE_T regionSize, 
                                           const std::vector<float>& targetValues) {
        std::vector<uintptr_t> foundAddresses;
        
        // 메모리 영역 읽기
        std::vector<uint8_t> buffer(regionSize);
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(baseAddress), 
                              buffer.data(), regionSize, &bytesRead)) {
            return foundAddresses;
        }
        
        // 4바이트씩 float 값으로 해석하여 스캔
        for (SIZE_T i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
            float value = *reinterpret_cast<const float*>(&buffer[i]);
            
            // 값이 유효한 범위인지 확인
            if (value > 0.0f && value < 1000.0f && std::isfinite(value)) {
                // 목표 값들과 비교
                for (float targetFPS : targetValues) {
                    if (abs(value - targetFPS) < 0.1f) {
                        foundAddresses.push_back(baseAddress + i);
                        break;
                    }
                }
            }
        }
        
        return foundAddresses;
    }
};

int main() {
    std::wcout << L"=== FPS 값 읽기 도구 ===" << std::endl;
    std::wcout << L"게임의 FPS 값을 메모리에서 찾아 읽어옵니다." << std::endl;
    
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
    
    FPSReader fpsReader;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 프로세스 이름을 입력하세요 (예: notepad.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 프로세스에 연결
    if (!fpsReader.AttachToProcess(processName)) {
        std::wcout << L"프로세스 연결 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메뉴
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. FPS 값 스캔" << std::endl;
        std::wcout << L"2. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1: {
                auto addresses = fpsReader.ScanForFPSValues();
                
                if (!addresses.empty()) {
                    std::wcout << L"\n발견된 주소들:" << std::endl;
                    for (size_t i = 0; i < std::min<size_t>(addresses.size(), 10); ++i) {
                        float fps;
                        if (fpsReader.ReadFPSValue(addresses[i], fps)) {
                            std::wcout << L"주소 " << i+1 << L": 0x" << std::hex << addresses[i] 
                                       << L" = " << std::dec << std::fixed << std::setprecision(2) << fps << std::endl;
                        }
                    }
                    
                    std::wcout << L"\nFPS 모니터링을 시작하시겠습니까? (y/n): ";
                    wchar_t monitor;
                    std::wcin >> monitor;
                    
                    if (monitor == L'y' || monitor == L'Y') {
                        fpsReader.MonitorFPS(addresses);
                    }
                } else {
                    std::wcout << L"FPS 값을 찾을 수 없습니다." << std::endl;
                    std::wcout << L"게임이 실행 중이고 FPS가 표시되는 상태인지 확인하세요." << std::endl;
                }
                break;
            }
            
            case 2:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}