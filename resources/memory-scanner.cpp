/*
 * 범용 메모리 스캐너
 *
 * 이 도구는 대상 프로세스의 메모리에서 다양한 데이터 유형을 스캔하는 기능을 제공합니다.
 * 게임 해킹, 리버스 엔지니어링 및 메모리 분석에 사용할 수 있습니다.
 *
 * 기능:
 * - 이름으로 대상 프로세스에 연결.
 * - 정수, 부동 소수점, 문자열 및 바이트 배열 값 스캔.
 * - 값 변경(예: 증가, 감소, 변경 없음)을 기반으로 스캔 결과 필터링.
 * - 주어진 주소에서 메모리 읽기 및 쓰기.
 * - 기본 오류 처리 및 프로세스 정보.
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <map>
#include <codecvt>
#include <locale>

// Helper function to convert wstring to string
std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// Helper function to convert string to wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

class MemoryScanner {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    std::map<uintptr_t, std::vector<uint8_t>> previousScanResults; // 값 변경 필터링용

public:
    MemoryScanner() : processHandle(nullptr), processId(0) {}

    ~MemoryScanner() {
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }

    bool AttachToProcess(const std::wstring& targetProcess) {
        processName = targetProcess;

        // 프로세스 ID 찾기
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            std::wcout << L"프로세스 스냅샷 생성 실패." << std::endl;
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

        // 필요한 권한으로 프로세스 핸들 열기
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }

        std::wcout << L"프로세스에 성공적으로 연결됨: " << processName
                   << L" (PID: " << processId << L")" << std::endl;
        return true;
    }

    // 일반 스캔 함수
    template<typename T>
    std::vector<uintptr_t> ScanMemory(const T& value, const std::vector<uintptr_t>& addressesToFilter = {}) {
        std::vector<uintptr_t> foundAddresses;
        
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

        std::wcout << L"값 스캔 시작: " << value << L"..." << std::endl;

        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            // 커밋되고 읽기 가능한 메모리 영역만 스캔
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || 
                 mbi.Protect == PAGE_READONLY ||
                 mbi.Protect == PAGE_EXECUTE_READ ||
                 mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                
                std::vector<uint8_t> buffer(mbi.RegionSize);
                SIZE_T bytesRead;

                if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(currentAddress), buffer.data(), mbi.RegionSize, &bytesRead)) {
                    for (SIZE_T i = 0; i <= bytesRead - sizeof(T); ++i) {
                        T currentValue = *reinterpret_cast<const T*>(&buffer[i]);
                        if (currentValue == value) {
                            uintptr_t foundAddr = currentAddress + i;
                            if (addressesToFilter.empty() || 
                                std::find(addressesToFilter.begin(), addressesToFilter.end(), foundAddr) != addressesToFilter.end()) {
                                foundAddresses.push_back(foundAddr);
                            }
                        }
                    }
                }
            }
            currentAddress += mbi.RegionSize;
        }
        std::wcout << L"스캔 완료. " << foundAddresses.size() << L"개 주소 발견." << std::endl;
        return foundAddresses;
    }

    // 바이트 배열(패턴) 특화 스캔
    std::vector<uintptr_t> ScanMemoryPattern(const std::vector<uint8_t>& pattern, const std::vector<bool>& mask, const std::vector<uintptr_t>& addressesToFilter = {}) {
        std::vector<uintptr_t> results;

        if (pattern.empty() || pattern.size() != mask.size()) {
            std::wcout << L"유효하지 않은 패턴 또는 마스크." << std::endl;
            return results;
        }

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        uintptr_t currentAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
        uintptr_t maxAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

        std::wcout << L"패턴 스캔 시작..." << std::endl;

        while (currentAddress < maxAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)) == 0) {
                break;
            }

            if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                std::vector<uint8_t> buffer(mbi.RegionSize);
                SIZE_T bytesRead;

                if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(currentAddress), buffer.data(), mbi.RegionSize, &bytesRead)) {
                    for (SIZE_T i = 0; i <= bytesRead - pattern.size(); ++i) {
                        bool match = true;
                        for (SIZE_T j = 0; j < pattern.size(); ++j) {
                            if (mask[j] && buffer[i + j] != pattern[j]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            uintptr_t foundAddr = currentAddress + i;
                            if (addressesToFilter.empty() || 
                                std::find(addressesToFilter.begin(), addressesToFilter.end(), foundAddr) != addressesToFilter.end()) {
                                results.push_back(foundAddr);
                            }
                        }
                    }
                }
            }
            currentAddress += mbi.RegionSize;
        }
        std::wcout << L"패턴 스캔 완료. " << results.size() << L"개 주소 발견." << std::endl;
        return results;
    }

    // 특정 주소에서 메모리 읽기
    template<typename T>
    bool ReadMemory(uintptr_t address, T& value) {
        SIZE_T bytesRead;
        return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T);
    }

    // 특정 주소에 메모리 쓰기
    template<typename T>
    bool WriteMemory(uintptr_t address, const T& value) {
        SIZE_T bytesWritten;
        DWORD oldProtect;
        // 쓰기 허용을 위해 메모리 보호 임시 변경
        if (!VirtualProtectEx(processHandle, reinterpret_cast<LPVOID>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::wcout << L"쓰기 위해 메모리 보호 변경 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        BOOL success = WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &bytesWritten);
        // 원래 메모리 보호 복원
        DWORD tempProtect;
        VirtualProtectEx(processHandle, reinterpret_cast<LPVOID>(address), sizeof(T), oldProtect, &tempProtect);
        return success && bytesWritten == sizeof(T);
    }

    // 값 변경에 따라 결과 필터링
    template<typename T>
    std::vector<uintptr_t> FilterByChange(const std::vector<uintptr_t>& currentAddresses, 
                                          const std::string& changeType) {
        std::vector<uintptr_t> filteredAddresses;
        
        if (currentAddresses.empty()) {
            std::wcout << L"필터링할 주소가 없습니다." << std::endl;
            return filteredAddresses;
        }

        std::wcout << L"" << currentAddresses.size() << L"개 주소를 변경 유형: " << StringToWString(changeType) << L"로 필터링 중..." << std::endl;

        for (uintptr_t addr : currentAddresses) {
            T currentValue;
            if (ReadMemory(addr, currentValue)) {
                // 이전 스캔 결과에 이 주소가 있었는지 확인
                if (previousScanResults.count(addr)) {
                    T previousValue = *reinterpret_cast<T*>(previousScanResults[addr].data());

                    bool include = false;
                    if (changeType == "unchanged" && currentValue == previousValue) {
                        include = true;
                    } else if (changeType == "increased" && currentValue > previousValue) {
                        include = true;
                    } else if (changeType == "decreased" && currentValue < previousValue) {
                        include = true;
                    } else if (changeType == "changed" && currentValue != previousValue) {
                        include = true;
                    }

                    if (include) {
                        filteredAddresses.push_back(addr);
                    }
                }
            }
        }

        // 다음 필터링 작업을 위해 이전 스캔 결과 업데이트
        previousScanResults.clear();
        for (uintptr_t addr : filteredAddresses) {
            T value;
            if (ReadMemory(addr, value)) {
                std::vector<uint8_t> bytes(sizeof(T));
                memcpy(bytes.data(), &value, sizeof(T));
                previousScanResults[addr] = bytes;
            }
        }

        std::wcout << L"" << filteredAddresses.size() << L"개 주소로 필터링됨." << std::endl;
        return filteredAddresses;
    }

    // 필터링을 위해 현재 스캔 결과 저장
    template<typename T>
    void StoreCurrentResults(const std::vector<uintptr_t>& addresses) {
        previousScanResults.clear();
        for (uintptr_t addr : addresses) {
            T value;
            if (ReadMemory(addr, value)) {
                std::vector<uint8_t> bytes(sizeof(T));
                memcpy(bytes.data(), &value, sizeof(T));
                previousScanResults[addr] = bytes;
            }
        }
    }

    void ShowAddresses(const std::vector<uintptr_t>& addresses, int limit = 10) {
        if (addresses.empty()) {
            std::wcout << L"표시할 주소가 없습니다." << std::endl;
            return;
        }
        std::wcout << L"" << addresses.size() << L"개 주소 중 최대 " << limit << L"개 표시:" << std::endl;
        for (size_t i = 0; i < std::min((size_t)limit, addresses.size()); ++i) {
            std::wcout << L"  0x" << std::hex << addresses[i] << std::dec << std::endl;
        }
    }
};

// 메인 애플리케이션 루프
int main() {
    std::wcout << L"=== 범용 메모리 스캐너 ===" << std::endl;
    std::wcout << L"프로세스에 연결하여 메모리를 스캔합니다." << std::endl;

    MemoryScanner scanner;
    std::vector<uintptr_t> currentAddresses;

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

    // 프로세스 이름 입력
    std::wcout << L"\n대상 프로세스 이름 (예: notepad.exe): ";
    std::wstring processName;
    std::wcin >> processName;

    if (!scanner.AttachToProcess(processName)) {
        std::wcout << L"프로세스 연결 실패." << std::endl;
        std::wcout << L"계속하려면 아무 키나 누르세요.";
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }

    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 첫 번째 스캔 (정수)" << std::endl;
        std::wcout << L"2. 다음 스캔 (정수) - 변경 없음" << std::endl;
        std::wcout << L"3. 다음 스캔 (정수) - 증가" << std::endl;
        std::wcout << L"4. 다음 스캔 (정수) - 감소" << std::endl;
        std::wcout << L"5. 다음 스캔 (정수) - 변경됨" << std::endl;
        std::wcout << L"6. 특정 정수 값 스캔" << std::endl;
        std::wcout << L"7. 메모리 읽기/쓰기 (정수)" << std::endl;
        std::wcout << L"8. 현재 주소 표시" << std::endl;
        std::wcout << L"9. 종료" << std::endl;
        std::wcout << L"선택: ";

        int choice;
        std::wcin >> choice;

        switch (choice) {
            case 1: {
                int value;
                std::wcout << L"초기 정수 값 입력: ";
                std::wcin >> value;
                currentAddresses = scanner.ScanMemory<int>(value);
                scanner.StoreCurrentResults<int>(currentAddresses);
                scanner.ShowAddresses(currentAddresses);
                break;
            }
            case 2: {
                currentAddresses = scanner.FilterByChange<int>(currentAddresses, "unchanged");
                scanner.ShowAddresses(currentAddresses);
                break;
            }
            case 3: {
                currentAddresses = scanner.FilterByChange<int>(currentAddresses, "increased");
                scanner.ShowAddresses(currentAddresses);
                break;
            }
            case 4: {
                currentAddresses = scanner.FilterByChange<int>(currentAddresses, "decreased");
                scanner.ShowAddresses(currentAddresses);
                break;
            }
            case 5: {
                currentAddresses = scanner.FilterByChange<int>(currentAddresses, "changed");
                scanner.ShowAddresses(currentAddresses);
                break;
            }
            case 6: {
                int value;
                std::wcout << L"스캔할 정수 값 입력: ";
                std::wcin >> value;
                currentAddresses = scanner.ScanMemory<int>(value, currentAddresses);
                scanner.StoreCurrentResults<int>(currentAddresses);
                scanner.ShowAddresses(currentAddresses);
                break;
            }
            case 7: {
                std::wcout << L"주소 입력 (16진수, 예: 0x12345678): ";
                uintptr_t addr;
                std::wcin >> std::hex >> addr >> std::dec;
                
                std::wcout << L"쓸 새 정수 값 입력: ";
                int newValue;
                std::wcin >> newValue;

                int oldValue;
                if (scanner.ReadMemory<int>(addr, oldValue)) {
                    std::wcout << L"0x" << std::hex << addr << L"의 이전 값: " << std::dec << oldValue << std::endl;
                }

                if (scanner.WriteMemory<int>(addr, newValue)) {
                    std::wcout << L"0x" << std::hex << addr << L"에 " << std::dec << newValue << L" 쓰기 성공." << std::endl;
                }
                else {
                    std::wcout << L"0x" << std::hex << addr << L"에 쓰기 실패." << std::dec << std::endl;
                }
                break;
            }
            case 8: {
                scanner.ShowAddresses(currentAddresses, 20);
                break;
            }
            case 9:
                std::wcout << L"스캐너 종료." << std::endl;
                return 0;
            default:
                std::wcout << L"잘못된 선택입니다. 다시 시도하세요." << std::endl;
                break;
        }
    }

    return 0;
}