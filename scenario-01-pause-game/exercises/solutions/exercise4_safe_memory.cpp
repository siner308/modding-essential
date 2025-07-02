/*
 * Exercise 4: 안전한 메모리 조작
 * 
 * 문제: 게임 크래시를 방지하는 안전한 메모리 읽기/쓰기 함수를 작성하세요.
 * 
 * 학습 목표:
 * - 예외 처리 (SEH - Structured Exception Handling)
 * - 메모리 보호 속성 확인
 * - 안전한 메모리 접근 패턴
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <string>

class SafeMemoryManager {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    // 메모리 접근 통계
    struct AccessStats {
        size_t successfulReads = 0;
        size_t failedReads = 0;
        size_t successfulWrites = 0;
        size_t failedWrites = 0;
        size_t protectionViolations = 0;
        size_t accessViolations = 0;
    };
    
    AccessStats stats;
    
    // 메모리 영역 캐시
    struct MemoryRegion {
        uintptr_t baseAddress;
        size_t size;
        DWORD protection;
        bool isReadable;
        bool isWritable;
        bool isExecutable;
    };
    
    std::map<uintptr_t, MemoryRegion> regionCache;

public:
    SafeMemoryManager() : processHandle(nullptr), processId(0) {}
    
    ~SafeMemoryManager() {
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
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE |
                                  PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
                                  FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        std::wcout << L"안전한 메모리 관리자 초기화 완료" << std::endl;
        return true;
    }
    
    enum class MemoryAccessResult {
        Success,
        InvalidAddress,
        AccessViolation,
        ProtectionViolation,
        PartialAccess,
        ProcessNotFound,
        UnknownError
    };
    
    template<typename T>
    MemoryAccessResult SafeRead(uintptr_t address, T& value) {
        return SafeRead(address, &value, sizeof(T));
    }
    
    MemoryAccessResult SafeRead(uintptr_t address, void* buffer, size_t size) {
        if (!IsValidAddress(address) || !buffer || size == 0) {
            stats.failedReads++;
            return MemoryAccessResult::InvalidAddress;
        }
        
        // 메모리 영역 보호 속성 확인
        auto region = GetMemoryRegion(address);
        if (!region.isReadable) {
            stats.protectionViolations++;
            return MemoryAccessResult::ProtectionViolation;
        }
        
        MemoryAccessResult result = MemoryAccessResult::UnknownError;
        
        __try {
            SIZE_T bytesRead = 0;
            BOOL success = ReadProcessMemory(processHandle,
                                           reinterpret_cast<LPCVOID>(address),
                                           buffer, size, &bytesRead);
            
            if (success && bytesRead == size) {
                stats.successfulReads++;
                result = MemoryAccessResult::Success;
            } else if (bytesRead > 0) {
                stats.successfulReads++;
                result = MemoryAccessResult::PartialAccess;
            } else {
                stats.failedReads++;
                result = MemoryAccessResult::AccessViolation;
            }
        }
        __except (ExceptionFilter(GetExceptionCode(), address, size, false)) {
            stats.accessViolations++;
            result = MemoryAccessResult::AccessViolation;
        }
        
        return result;
    }
    
    template<typename T>
    MemoryAccessResult SafeWrite(uintptr_t address, const T& value) {
        return SafeWrite(address, &value, sizeof(T));
    }
    
    MemoryAccessResult SafeWrite(uintptr_t address, const void* buffer, size_t size) {
        if (!IsValidAddress(address) || !buffer || size == 0) {
            stats.failedWrites++;
            return MemoryAccessResult::InvalidAddress;
        }
        
        // 메모리 영역 보호 속성 확인
        auto region = GetMemoryRegion(address);
        if (!region.isWritable) {
            stats.protectionViolations++;
            return MemoryAccessResult::ProtectionViolation;
        }
        
        MemoryAccessResult result = MemoryAccessResult::UnknownError;
        DWORD oldProtection = 0;
        bool protectionChanged = false;
        
        __try {
            // 필요한 경우 메모리 보호 속성 변경
            if (!(region.protection & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE))) {
                if (VirtualProtectEx(processHandle, reinterpret_cast<LPVOID>(address),
                                   size, PAGE_READWRITE, &oldProtection)) {
                    protectionChanged = true;
                }
            }
            
            SIZE_T bytesWritten = 0;
            BOOL success = WriteProcessMemory(processHandle,
                                            reinterpret_cast<LPVOID>(address),
                                            buffer, size, &bytesWritten);
            
            if (success && bytesWritten == size) {
                stats.successfulWrites++;
                result = MemoryAccessResult::Success;
            } else if (bytesWritten > 0) {
                stats.successfulWrites++;
                result = MemoryAccessResult::PartialAccess;
            } else {
                stats.failedWrites++;
                result = MemoryAccessResult::AccessViolation;
            }
            
            // 메모리 보호 속성 복원
            if (protectionChanged) {
                DWORD temp;
                VirtualProtectEx(processHandle, reinterpret_cast<LPVOID>(address),
                               size, oldProtection, &temp);
            }
        }
        __except (ExceptionFilter(GetExceptionCode(), address, size, true)) {
            stats.accessViolations++;
            result = MemoryAccessResult::AccessViolation;
            
            // 예외 발생 시에도 보호 속성 복원
            if (protectionChanged) {
                DWORD temp;
                VirtualProtectEx(processHandle, reinterpret_cast<LPVOID>(address),
                               size, oldProtection, &temp);
            }
        }
        
        return result;
    }
    
    bool IsValidAddress(uintptr_t address) {
        if (address == 0) {
            return false;
        }
        
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(address),
                          &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        
        return mbi.State == MEM_COMMIT;
    }
    
    MemoryRegion GetMemoryRegion(uintptr_t address) {
        // 캐시에서 먼저 확인
        auto it = regionCache.find(address);
        if (it != regionCache.end()) {
            return it->second;
        }
        
        MemoryRegion region = {};
        MEMORY_BASIC_INFORMATION mbi;
        
        if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(address),
                          &mbi, sizeof(mbi)) != 0) {
            region.baseAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            region.size = mbi.RegionSize;
            region.protection = mbi.Protect;
            
            // 접근 권한 분석
            region.isReadable = (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
                                              PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
            region.isWritable = (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0;
            region.isExecutable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                                                PAGE_EXECUTE_READWRITE)) != 0;
            
            // 캐시에 저장
            regionCache[address] = region;
        }
        
        return region;
    }
    
    std::vector<uintptr_t> ScanMemoryPattern(const std::vector<uint8_t>& pattern,
                                           const std::vector<bool>& mask) {
        std::vector<uintptr_t> results;
        
        if (pattern.size() != mask.size() || pattern.empty()) {
            return results;
        }
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
        
        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }
            
            if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                auto regionResults = ScanRegionForPattern(currentAddress, mbi.RegionSize,
                                                        pattern, mask);
                results.insert(results.end(), regionResults.begin(), regionResults.end());
            }
            
            currentAddress += mbi.RegionSize;
        }
        
        return results;
    }
    
    std::vector<uintptr_t> ScanRegionForPattern(uintptr_t baseAddress, size_t regionSize,
                                              const std::vector<uint8_t>& pattern,
                                              const std::vector<bool>& mask) {
        std::vector<uintptr_t> results;
        
        // 안전한 청크 단위로 읽기 (1MB씩)
        const size_t chunkSize = 1024 * 1024;
        std::vector<uint8_t> buffer(chunkSize);
        
        auto readResult = SafeRead(baseAddress, buffer.data(), regionSize);
        if (readResult == MemoryAccessResult::Success ||
            readResult == MemoryAccessResult::PartialAccess) {
            
            // 패턴 검색
            for (size_t i = 0; i <= regionSize - pattern.size(); ++i) {
                bool match = true;
                for (size_t j = 0; j < pattern.size(); ++j) {
                    if (mask[j] && buffer[i + j] != pattern[j]) {
                        match = false;
                        break;
                    }
                }
                
                if (match) {
                    results.push_back(baseAddress + i);
                }
            }
        }
        
        return results;
    }
    
    void ShowStatistics() {
        std::wcout << L"\n=== 메모리 접근 통계 ===" << std::endl;
        std::wcout << L"성공한 읽기: " << stats.successfulReads << std::endl;
        std::wcout << L"실패한 읽기: " << stats.failedReads << std::endl;
        std::wcout << L"성공한 쓰기: " << stats.successfulWrites << std::endl;
        std::wcout << L"실패한 쓰기: " << stats.failedWrites << std::endl;
        std::wcout << L"보호 위반: " << stats.protectionViolations << std::endl;
        std::wcout << L"접근 위반: " << stats.accessViolations << std::endl;
        
        size_t totalOperations = stats.successfulReads + stats.failedReads +
                               stats.successfulWrites + stats.failedWrites;
        if (totalOperations > 0) {
            double successRate = static_cast<double>(stats.successfulReads + stats.successfulWrites) /
                               totalOperations * 100.0;
            std::wcout << std::fixed << std::setprecision(1) << L"성공률: " << successRate << L"%" << std::endl;
        }
    }
    
    void ClearCache() {
        regionCache.clear();
        std::wcout << L"메모리 영역 캐시가 지워졌습니다." << std::endl;
    }
    
    std::wstring GetResultDescription(MemoryAccessResult result) {
        switch (result) {
            case MemoryAccessResult::Success:
                return L"성공";
            case MemoryAccessResult::InvalidAddress:
                return L"잘못된 주소";
            case MemoryAccessResult::AccessViolation:
                return L"접근 위반";
            case MemoryAccessResult::ProtectionViolation:
                return L"보호 위반";
            case MemoryAccessResult::PartialAccess:
                return L"부분 접근";
            case MemoryAccessResult::ProcessNotFound:
                return L"프로세스를 찾을 수 없음";
            case MemoryAccessResult::UnknownError:
                return L"알 수 없는 오류";
            default:
                return L"정의되지 않은 결과";
        }
    }
    
    void TestMemoryOperations() {
        std::wcout << L"\n=== 메모리 접근 테스트 ===" << std::endl;
        
        // 1. 잘못된 주소 테스트
        std::wcout << L"1. 잘못된 주소 테스트..." << std::endl;
        int value;
        auto result = SafeRead<int>(0x0, value);
        std::wcout << L"   결과: " << GetResultDescription(result) << std::endl;
        
        // 2. 유효한 주소 검색
        std::wcout << L"2. 유효한 메모리 영역 검색..." << std::endl;
        auto validAddresses = FindValidMemoryRegions();
        std::wcout << L"   발견된 영역 수: " << validAddresses.size() << std::endl;
        
        // 3. 패턴 스캔 테스트
        if (!validAddresses.empty()) {
            std::wcout << L"3. 패턴 스캔 테스트..." << std::endl;
            std::vector<uint8_t> pattern = {0x48, 0x89, 0x5C, 0x24}; // mov [rsp+?], rbx
            std::vector<bool> mask = {true, true, true, false};
            
            auto addresses = ScanMemoryPattern(pattern, mask);
            std::wcout << L"   패턴 발견 횟수: " << addresses.size() << std::endl;
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
    
    int ExceptionFilter(DWORD exceptionCode, uintptr_t address, size_t size, bool isWrite) {
        std::wcout << L"메모리 접근 예외 발생:" << std::endl;
        std::wcout << L"  주소: 0x" << std::hex << address << std::dec << std::endl;
        std::wcout << L"  크기: " << size << L" 바이트" << std::endl;
        std::wcout << L"  작업: " << (isWrite ? L"쓰기" : L"읽기") << std::endl;
        std::wcout << L"  예외 코드: 0x" << std::hex << exceptionCode << std::dec << std::endl;
        
        switch (exceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:
                std::wcout << L"  유형: 접근 위반" << std::endl;
                break;
            case EXCEPTION_GUARD_PAGE:
                std::wcout << L"  유형: 가드 페이지 접근" << std::endl;
                break;
            case EXCEPTION_IN_PAGE_ERROR:
                std::wcout << L"  유형: 페이지 오류" << std::endl;
                break;
            default:
                std::wcout << L"  유형: 알 수 없는 예외" << std::endl;
                break;
        }
        
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    std::vector<MemoryRegion> FindValidMemoryRegions() {
        std::vector<MemoryRegion> regions;
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);
        
        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress),
                              &mbi, sizeof(mbi)) == 0) {
                break;
            }
            
            if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                MemoryRegion region;
                region.baseAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
                region.size = mbi.RegionSize;
                region.protection = mbi.Protect;
                region.isReadable = (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
                                                  PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
                region.isWritable = (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0;
                region.isExecutable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                                                    PAGE_EXECUTE_READWRITE)) != 0;
                
                regions.push_back(region);
            }
            
            currentAddress += mbi.RegionSize;
        }
        
        return regions;
    }
};

int main() {
    std::wcout << L"=== 안전한 메모리 관리자 ===" << std::endl;
    std::wcout << L"크래시 없는 안전한 메모리 접근을 제공합니다." << std::endl;
    
    SafeMemoryManager memManager;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 초기화
    if (!memManager.Initialize(processName)) {
        std::wcout << L"초기화 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 메모리 읽기 테스트" << std::endl;
        std::wcout << L"2. 메모리 쓰기 테스트" << std::endl;
        std::wcout << L"3. 패턴 스캔" << std::endl;
        std::wcout << L"4. 메모리 접근 테스트" << std::endl;
        std::wcout << L"5. 통계 보기" << std::endl;
        std::wcout << L"6. 캐시 지우기" << std::endl;
        std::wcout << L"7. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1: {
                std::wcout << L"읽을 주소를 입력하세요 (16진수, 0x 접두사 포함): ";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                
                int value;
                auto result = memManager.SafeRead(address, value);
                std::wcout << L"결과: " << memManager.GetResultDescription(result) << std::endl;
                
                if (result == SafeMemoryManager::MemoryAccessResult::Success) {
                    std::wcout << L"읽은 값: " << value << std::endl;
                }
                break;
            }
            
            case 2: {
                std::wcout << L"쓸 주소를 입력하세요 (16진수, 0x 접두사 포함): ";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                
                std::wcout << L"쓸 값을 입력하세요: ";
                int value;
                std::wcin >> value;
                
                auto result = memManager.SafeWrite(address, value);
                std::wcout << L"결과: " << memManager.GetResultDescription(result) << std::endl;
                break;
            }
            
            case 3: {
                std::wcout << L"간단한 패턴 스캔을 실행합니다..." << std::endl;
                std::vector<uint8_t> pattern = {0x48, 0x89, 0x5C, 0x24};
                std::vector<bool> mask = {true, true, true, false};
                
                auto addresses = memManager.ScanMemoryPattern(pattern, mask);
                std::wcout << L"패턴을 " << addresses.size() << L"곳에서 발견했습니다." << std::endl;
                
                for (size_t i = 0; i < std::min<size_t>(addresses.size(), 5); ++i) {
                    std::wcout << L"  0x" << std::hex << addresses[i] << std::dec << std::endl;
                }
                break;
            }
            
            case 4:
                memManager.TestMemoryOperations();
                break;
                
            case 5:
                memManager.ShowStatistics();
                break;
                
            case 6:
                memManager.ClearCache();
                break;
                
            case 7:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}