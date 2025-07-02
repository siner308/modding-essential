/*
 * Exercise 1: FPS 값 스캔
 * 
 * 문제: 메모리에서 FPS 제한 값을 찾는 스캐너를 작성하세요.
 * 
 * 학습 목표:
 * - AOB (Array of Bytes) 스캔 기법
 * - 메모리 패턴 인식
 * - 동적 주소 찾기
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cmath> // For std::isfinite

class FPSScanner {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    struct ScanResult {
        uintptr_t address;
        float value;
        int matchCount;
        bool isStable;
    };

public:
    FPSScanner() : processHandle(nullptr), processId(0) {}
    
    ~FPSScanner() {
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }
    
    bool Initialize(const std::wstring& targetProcess) {
        processName = targetProcess;
        
        // 프로세스 찾기
        if (!FindProcess()) {
            return false;
        }
        
        // 프로세스 핸들 열기
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, 
                                  FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        std::wcout << L"FPS 스캐너 초기화 완료" << std::endl;
        return true;
    }
    
    std::vector<ScanResult> ScanForFPSLimit() {
        std::wcout << L"FPS 제한 값 스캔 시작..." << std::endl;
        
        // 1차 스캔: 일반적인 FPS 값들
        std::vector<float> commonFPS = {30.0f, 60.0f, 75.0f, 90.0f, 120.0f, 144.0f, 165.0f, 240.0f};
        std::vector<ScanResult> candidates;
        
        for (float targetFPS : commonFPS) {
            auto addresses = ScanMemoryForFloat(targetFPS);
            std::wcout << L"FPS " << targetFPS << L": " << addresses.size() << L"개 주소 발견" << std::endl;
            
            for (uintptr_t addr : addresses) {
                ScanResult result;
                result.address = addr;
                result.value = targetFPS;
                result.matchCount = 1;
                result.isStable = false;
                candidates.push_back(result);
            }
        }
        
        if (candidates.empty()) {
            std::wcout << L"FPS 제한 값을 찾을 수 없습니다." << std::endl;
            return candidates;
        }
        
        std::wcout << L"총 " << candidates.size() << L"개의 후보 주소 발견" << std::endl;
        std::wcout << L"주소 검증 중..." << std::endl;
        
        // 2차 검증: 값의 안정성 확인
        auto validatedResults = ValidateAddresses(candidates);
        
        std::wcout << L"검증 완료. " << validatedResults.size() << L"개의 유효한 주소" << std::endl;
        return validatedResults;
    }
    
    std::vector<uintptr_t> ScanMemoryForFloat(float targetValue) {
        std::vector<uintptr_t> foundAddresses;
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
        
        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }
            
            // 쓰기 가능한 메모리 영역만 스캔 (FPS 설정 값이 저장될 가능성이 높음)
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                auto addresses = ScanRegionForFloat(currentAddress, mbi.RegionSize, targetValue);
                foundAddresses.insert(foundAddresses.end(), addresses.begin(), addresses.end());
            }
            
            currentAddress += mbi.RegionSize;
        }
        
        return foundAddresses;
    }
    
    std::vector<uintptr_t> ScanRegionForFloat(uintptr_t baseAddress, SIZE_T regionSize, float targetValue) {
        std::vector<uintptr_t> foundAddresses;
        
        // 메모리 읽기
        std::vector<uint8_t> buffer(regionSize);
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(baseAddress), 
                              buffer.data(), regionSize, &bytesRead)) {
            return foundAddresses;
        }
        
        // 4바이트 정렬된 위치에서 float 값 검색
        for (SIZE_T i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
            float value = *reinterpret_cast<const float*>(&buffer[i]);
            
            // 목표 값과 비교 (약간의 오차 허용)
            if (abs(value - targetValue) < 0.01f) {
                foundAddresses.push_back(baseAddress + i);
            }
        }
        
        return foundAddresses;
    }
    
    std::vector<ScanResult> ValidateAddresses(const std::vector<ScanResult>& candidates) {
        std::vector<ScanResult> validatedResults;
        
        for (const auto& candidate : candidates) {
            if (IsValidFPSAddress(candidate.address)) {
                ScanResult validated = candidate;
                validated.isStable = IsAddressStable(candidate.address);
                validatedResults.push_back(validated);
            }
        }
        
        return validatedResults;
    }
    
    bool IsValidFPSAddress(uintptr_t address) {
        float value;
        SIZE_T bytesRead;
        
        // 값 읽기 가능한지 확인
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), 
                              &value, sizeof(float), &bytesRead) || bytesRead != sizeof(float)) {
            return false;
        }
        
        // 합리적인 FPS 범위인지 확인
        if (value < 10.0f || value > 1000.0f || !std::isfinite(value)) {
            return false;
        }
        
        // 쓰기 테스트 (원본 값 백업 후 복원)
        float originalValue = value;
        float testValue = originalValue + 1.0f;
        
        SIZE_T bytesWritten;
        if (WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), 
                              &testValue, sizeof(float), &bytesWritten) && bytesWritten == sizeof(float)) {
            
            // 값이 변경되었는지 확인
            float readBackValue;
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), 
                                 &readBackValue, sizeof(float), &bytesRead) && bytesRead == sizeof(float)) {
                
                // 원본 값 복원
                WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), 
                                 &originalValue, sizeof(float), &bytesWritten);
                
                // 테스트 값이 정확히 쓰여졌는지 확인
                return abs(readBackValue - testValue) < 0.01f;
            }
        }
        
        return false;
    }
    
    bool IsAddressStable(uintptr_t address) {
        std::vector<float> readings;
        
        // 1초 동안 10번 읽어서 안정성 확인
        for (int i = 0; i < 10; ++i) {
            float value;
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), 
                                 &value, sizeof(float), &bytesRead) && bytesRead == sizeof(float)) {
                readings.push_back(value);
            }
            
            Sleep(100); // 0.1초 대기
        }
        
        if (readings.size() < 5) {
            return false; // 읽기 실패가 너무 많음
        }
        
        // 값의 변동이 작은지 확인
        float minVal = *std::min_element(readings.begin(), readings.end());
        float maxVal = *std::max_element(readings.begin(), readings.end());
        
        return (maxVal - minVal) < 1.0f; // 1 FPS 이내 변동
    }
    
    bool SetFPSLimit(uintptr_t address, float newFPS) {
        if (newFPS != 0.0f && (newFPS < 10.0f || newFPS > 1000.0f)) {
            std::wcout << L"유효하지 않은 FPS 값입니다: " << newFPS << std::endl;
            return false;
        }
        
        // 무제한 FPS는 매우 큰 값으로 설정
        float actualFPS = (newFPS == 0.0f) ? 9999.0f : newFPS;
        
        SIZE_T bytesWritten;
        if (WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), 
                              &actualFPS, sizeof(float), &bytesWritten) && bytesWritten == sizeof(float)) {
            
            std::wcout << L"FPS 제한 변경 완료: " << newFPS;
            if (newFPS == 0.0f) {
                std::wcout << L" (무제한)";
            }
            std::wcout << std::endl;
            return true;
        } else {
            std::wcout << L"FPS 제한 변경 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
    }
    
    float ReadFPSValue(uintptr_t address) {
        float value = 0.0f;
        SIZE_T bytesRead;
        
        ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), 
                         &value, sizeof(float), &bytesRead);
        
        return value;
    }
    
    void MonitorFPS(uintptr_t address, int duration = 10) {
        std::wcout << L"FPS 모니터링 시작 (" << duration << L"초)..." << std::endl;
        std::wcout << L"주소: 0x" << std::hex << address << std::dec << std::endl;
        
        auto startTime = GetTickCount64();
        auto endTime = startTime + (duration * 1000);
        
        while (GetTickCount64() < endTime) {
            float fps = ReadFPSValue(address);
            
            std::wcout << L"\r현재 FPS 제한: " << std::fixed << std::setprecision(1) << fps;
            if (fps > 9000.0f) {
                std::wcout << L" (무제한)";
            }
            std::wcout << L"     ";
            
            Sleep(500); // 0.5초마다 업데이트
        }
        
        std::wcout << std::endl << L"모니터링 완료" << std::endl;
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
};

int main() {
    std::wcout << L"=== FPS 제한 스캐너 ===" << std::endl;
    std::wcout << L"게임의 FPS 제한 값을 찾고 수정할 수 있습니다." << std::endl;
    
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
    
    FPSScanner scanner;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 스캐너 초기화
    if (!scanner.Initialize(processName)) {
        std::wcout << L"스캐너 초기화 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    std::vector<FPSScanner::ScanResult> scanResults;
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. FPS 제한 스캔" << std::endl;
        std::wcout << L"2. FPS 제한 변경" << std::endl;
        std::wcout << L"3. FPS 모니터링" << std::endl;
        std::wcout << L"4. 스캔 결과 보기" << std::endl;
        std::wcout << L"5. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1: {
                scanResults = scanner.ScanForFPSLimit();
                
                if (!scanResults.empty()) {
                    std::wcout << L"\n스캔 결과:" << std::endl;
                    for (size_t i = 0; i < std::min<size_t>(scanResults.size(), 10); ++i) {
                        const auto& result = scanResults[i];
                        std::wcout << L"  " << i+1 << L". 주소: 0x" << std::hex << result.address 
                                   << L", 값: " << std::dec << std::fixed << std::setprecision(1) << result.value
                                   << L", 안정성: " << (result.isStable ? L"안정" : L"불안정") << std::endl;
                    }
                } else {
                    std::wcout << L"FPS 제한 값을 찾지 못했습니다." << std::endl;
                    std::wcout << L"게임이 실행 중이고 FPS 제한이 활성화되어 있는지 확인하세요." << std::endl;
                }
                break;
            }
            
            case 2: {
                if (scanResults.empty()) {
                    std::wcout << L"먼저 FPS 스캔을 실행하세요." << std::endl;
                    break;
                }
                
                std::wcout << L"변경할 주소를 선택하세요 (1-" << std::min<size_t>(scanResults.size(), 10) << L"): ";
                int index;
                std::wcin >> index;
                
                if (index < 1 || index > static_cast<int>(std::min<size_t>(scanResults.size(), 10))) {
                    std::wcout << L"잘못된 선택입니다." << std::endl;
                    break;
                }
                
                std::wcout << L"새로운 FPS 제한 값을 입력하세요 (0 = 무제한): ";
                float newFPS;
                std::wcin >> newFPS;
                
                scanner.SetFPSLimit(scanResults[index-1].address, newFPS);
                break;
            }
            
            case 3: {
                if (scanResults.empty()) {
                    std::wcout << L"먼저 FPS 스캔을 실행하세요." << std::endl;
                    break;
                }
                
                std::wcout << L"모니터링할 주소를 선택하세요 (1-" << std::min<size_t>(scanResults.size(), 10) << L"): ";
                int index;
                std::wcin >> index;
                
                if (index < 1 || index > static_cast<int>(std::min<size_t>(scanResults.size(), 10))) {
                    std::wcout << L"잘못된 선택입니다." << std::endl;
                    break;
                }
                
                scanner.MonitorFPS(scanResults[index-1].address);
                break;
            }
            
            case 4: {
                if (scanResults.empty()) {
                    std::wcout << L"스캔 결과가 없습니다." << std::endl;
                    break;
                }
                
                std::wcout << L"\n=== 스캔 결과 ===" << std::endl;
                for (size_t i = 0; i < scanResults.size(); ++i) {
                    const auto& result = scanResults[i];
                    float currentValue = scanner.ReadFPSValue(result.address);
                    
                    std::wcout << L"  " << i+1 << L". 주소: 0x" << std::hex << result.address << std::dec
                               << L", 현재 값: " << std::fixed << std::setprecision(1) << currentValue
                               << L", 안정성: " << (result.isStable ? L"안정" : L"불안정") << std::endl;
                }
                break;
            }
            
            case 5:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}