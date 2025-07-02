# Exercise Solutions - 고급 역공학 기법

이 폴더는 scenario-06-advanced-techniques의 연습문제 해답들을 포함합니다.

## 📋 연습문제 목록

### Exercise 1: 안티 디버그 우회
**문제**: 게임의 기본적인 안티 디버깅 기법을 탐지하고 우회하는 프로그램을 작성하세요.

**해답 파일**: `exercise1_anti_debug_bypass.cpp`

### Exercise 2: 코드 케이브 활용
**문제**: 실행 파일에서 사용되지 않는 공간을 찾아 커스텀 코드를 삽입하세요.

**해답 파일**: `exercise2_code_cave_injection.cpp`

### Exercise 3: 가상화 탐지
**문제**: 프로그램이 가상 머신에서 실행되는지 탐지하는 기법을 구현하세요.

**해답 파일**: `exercise3_vm_detection.cpp`

### Exercise 4: 패킹된 실행 파일 분석
**문제**: UPX로 패킹된 실행 파일을 메모리에서 덤프하여 분석하세요.

**해답 파일**: `exercise4_unpacking.cpp`

### Exercise 5: 난독화 해제
**문제**: 간단한 문자열 XOR 암호화를 탐지하고 해독하는 도구를 만드세요.

**해답 파일**: `exercise5_deobfuscation.cpp`

## 🛡️ 학습 목표

### 보안 분석
1. **안티 분석 기법**: 디버깅, VM 탐지, 패킹
2. **코드 난독화**: 문자열 암호화, 제어 흐름 난독화
3. **동적 분석**: 런타임 행동 분석
4. **정적 분석**: 바이너리 구조 분석

### 고급 시스템 프로그래밍
1. **PE 파일 형식**: 헤더, 섹션, 임포트/익스포트
2. **메모리 관리**: 가상 메모리, 메모리 보호
3. **프로세스 조작**: 인젝션, 후킹, 패칭
4. **어셈블리**: x64/x86 어셈블리 분석

## 🔍 핵심 기법

### 안티 디버그 탐지 및 우회
```cpp
class AntiDebugDetector {
public:
    // IsDebuggerPresent API 후킹
    static BOOL WINAPI HookedIsDebuggerPresent() {
        return FALSE; // 항상 디버거 없음으로 반환
    }
    
    // PEB의 BeingDebugged 플래그 우회
    static void BypassPEBFlag() {
        PPEB peb = GetPEB();
        if (peb) {
            peb->BeingDebugged = FALSE;
        }
    }
    
    // NtGlobalFlag 우회
    static void BypassNtGlobalFlag() {
        PPEB peb = GetPEB();
        if (peb) {
            peb->NtGlobalFlag &= ~(FLG_HEAP_ENABLE_TAIL_CHECK | 
                                  FLG_HEAP_ENABLE_FREE_CHECK | 
                                  FLG_HEAP_VALIDATE_PARAMETERS);
        }
    }
    
    // CheckRemoteDebuggerPresent 우회
    static BOOL WINAPI HookedCheckRemoteDebuggerPresent(HANDLE hProcess, PBOOL pbDebuggerPresent) {
        if (pbDebuggerPresent) {
            *pbDebuggerPresent = FALSE;
        }
        return TRUE;
    }
    
private:
    static PPEB GetPEB() {
#ifdef _WIN64
        return reinterpret_cast<PPEB>(__readgsqword(0x60));
#else
        return reinterpret_cast<PPEB>(__readfsdword(0x30));
#endif
    }
};
```

### 코드 케이브 탐지 및 활용
```cpp
class CodeCaveFinder {
public:
    struct CodeCave {
        uintptr_t address;
        size_t size;
        bool isExecutable;
    };
    
    static std::vector<CodeCave> FindCodeCaves(HANDLE hProcess, size_t minSize) {
        std::vector<CodeCave> caves;
        uintptr_t address = 0;
        
        while (true) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0) {
                break;
            }
            
            // 실행 가능한 메모리 영역 검사
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
                
                auto cave = ScanForCaveInRegion(hProcess, address, mbi.RegionSize, minSize);
                if (cave.size >= minSize) {
                    caves.push_back(cave);
                }
            }
            
            address += mbi.RegionSize;
        }
        
        return caves;
    }
    
    static bool InjectCodeIntoCave(HANDLE hProcess, const CodeCave& cave, 
                                  const std::vector<uint8_t>& code) {
        if (code.size() > cave.size) {
            return false;
        }
        
        // 메모리 보호 속성 변경
        DWORD oldProtect;
        if (!VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(cave.address), 
                             code.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return false;
        }
        
        // 코드 주입
        SIZE_T bytesWritten;
        bool success = WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(cave.address), 
                                        code.data(), code.size(), &bytesWritten);
        
        // 보호 속성 복원
        VirtualProtectEx(hProcess, reinterpret_cast<LPVOID>(cave.address), 
                        code.size(), oldProtect, &oldProtect);
        
        return success && bytesWritten == code.size();
    }
    
private:
    static CodeCave ScanForCaveInRegion(HANDLE hProcess, uintptr_t startAddr, 
                                      size_t regionSize, size_t minSize) {
        std::vector<uint8_t> buffer(regionSize);
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(startAddr), 
                              buffer.data(), regionSize, &bytesRead)) {
            return {0, 0, false};
        }
        
        // NULL 바이트 연속 영역 찾기
        size_t currentCaveSize = 0;
        uintptr_t currentCaveStart = 0;
        
        for (size_t i = 0; i < bytesRead; ++i) {
            if (buffer[i] == 0x00) {
                if (currentCaveSize == 0) {
                    currentCaveStart = startAddr + i;
                }
                currentCaveSize++;
            } else {
                if (currentCaveSize >= minSize) {
                    return {currentCaveStart, currentCaveSize, true};
                }
                currentCaveSize = 0;
            }
        }
        
        return {0, 0, false};
    }
};
```

### 가상화 환경 탐지
```cpp
class VMDetector {
public:
    static bool IsRunningInVM() {
        return CheckVMWare() || CheckVirtualBox() || CheckHyperV() || 
               CheckQEMU() || CheckXen() || CheckParallels();
    }
    
private:
    static bool CheckVMWare() {
        // VMware 특정 레지스트리 키 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\VMware, Inc.\\VMware Tools", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        
        // VMware 특정 서비스 확인
        SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
        if (scManager) {
            SC_HANDLE service = OpenServiceA(scManager, "VMTools", SERVICE_QUERY_STATUS);
            if (service) {
                CloseServiceHandle(service);
                CloseServiceHandle(scManager);
                return true;
            }
            CloseServiceHandle(scManager);
        }
        
        // CPUID를 통한 하이퍼바이저 탐지
        int cpuid[4];
        __cpuid(cpuid, 1);
        return (cpuid[2] >> 31) & 1; // 하이퍼바이저 비트
    }
    
    static bool CheckVirtualBox() {
        // VirtualBox Guest Additions 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\Oracle\\VirtualBox Guest Additions", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        
        // VirtualBox 특정 디바이스 확인
        HANDLE hDevice = CreateFileA("\\\\.\\VBoxMiniRdrDN", 
                                   GENERIC_READ, FILE_SHARE_READ, 
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(hDevice);
            return true;
        }
        
        return false;
    }
    
    static bool CheckHyperV() {
        // Hyper-V 특정 레지스트리 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }
    
    static bool CheckQEMU() {
        // QEMU 특정 하드웨어 확인
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) {
            return strstr(computerName, "QEMU") != nullptr;
        }
        return false;
    }
    
    static bool CheckXen() {
        // Xen 하이퍼바이저 탐지
        int cpuid[4];
        __cpuid(cpuid, 0x40000000);
        char vendor[13] = {0};
        memcpy(vendor, &cpuid[1], 4);
        memcpy(vendor + 4, &cpuid[2], 4);
        memcpy(vendor + 8, &cpuid[3], 4);
        return strcmp(vendor, "XenVMMXenVMM") == 0;
    }
    
    static bool CheckParallels() {
        // Parallels 특정 레지스트리 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\Parallels\\Parallels Tools", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }
};
```

### 메모리 덤프 및 언패킹
```cpp
class MemoryDumper {
public:
    static bool DumpProcessMemory(DWORD processId, const std::string& outputPath) {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess) {
            return false;
        }
        
        // 프로세스의 모든 모듈 열거
        HMODULE hModules[1024];
        DWORD cbNeeded;
        if (!EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
            CloseHandle(hProcess);
            return false;
        }
        
        DWORD moduleCount = cbNeeded / sizeof(HMODULE);
        
        for (DWORD i = 0; i < moduleCount; ++i) {
            MODULEINFO modInfo;
            if (GetModuleInformation(hProcess, hModules[i], &modInfo, sizeof(modInfo))) {
                std::string modulePath = outputPath + "_module_" + std::to_string(i) + ".bin";
                DumpMemoryRegion(hProcess, reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll), 
                               modInfo.SizeOfImage, modulePath);
            }
        }
        
        CloseHandle(hProcess);
        return true;
    }
    
    static bool DumpMemoryRegion(HANDLE hProcess, uintptr_t baseAddress, 
                                size_t size, const std::string& filePath) {
        std::vector<uint8_t> buffer(size);
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(baseAddress), 
                              buffer.data(), size, &bytesRead)) {
            return false;
        }
        
        std::ofstream file(filePath, std::ios::binary);
        if (!file) {
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
        return file.good();
    }
    
    // OEP (Original Entry Point) 찾기
    static uintptr_t FindOEP(HANDLE hProcess, uintptr_t baseAddress) {
        // IAT 후킹을 통한 OEP 탐지
        return SetOEPBreakpoint(hProcess, baseAddress);
    }
    
private:
    static uintptr_t SetOEPBreakpoint(HANDLE hProcess, uintptr_t baseAddress) {
        // 이 함수는 실제로는 더 복잡한 구현이 필요
        // 예: IAT에 브레이크포인트 설정, 언패킹 완료 탐지 등
        return baseAddress + 0x1000; // 예시 오프셋
    }
};
```

### 문자열 난독화 해제
```cpp
class StringDeobfuscator {
public:
    static std::vector<std::string> FindXOREncryptedStrings(const std::vector<uint8_t>& data) {
        std::vector<std::string> results;
        
        // 일반적인 XOR 키 범위 (1-255)
        for (int key = 1; key < 256; ++key) {
            auto decrypted = XORDecrypt(data, key);
            auto strings = ExtractPrintableStrings(decrypted);
            
            for (const auto& str : strings) {
                if (IsMeaningfulString(str)) {
                    results.push_back("Key " + std::to_string(key) + ": " + str);
                }
            }
        }
        
        return results;
    }
    
    static std::vector<uint8_t> XORDecrypt(const std::vector<uint8_t>& data, uint8_t key) {
        std::vector<uint8_t> result(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = data[i] ^ key;
        }
        return result;
    }
    
    static std::vector<std::string> ExtractPrintableStrings(const std::vector<uint8_t>& data) {
        std::vector<std::string> strings;
        std::string current;
        
        for (uint8_t byte : data) {
            if (byte >= 32 && byte <= 126) { // 인쇄 가능한 ASCII
                current += static_cast<char>(byte);
            } else {
                if (current.length() >= 4) { // 최소 4글자 이상
                    strings.push_back(current);
                }
                current.clear();
            }
        }
        
        if (current.length() >= 4) {
            strings.push_back(current);
        }
        
        return strings;
    }
    
private:
    static bool IsMeaningfulString(const std::string& str) {
        // 의미있는 문자열인지 판단하는 휴리스틱
        if (str.length() < 4) return false;
        
        // 영어 단어나 일반적인 패턴 확인
        static const std::vector<std::string> commonWords = {
            "the", "and", "for", "are", "but", "not", "you", "all", "can", "had", "her", "was", "one", "our",
            "error", "file", "system", "user", "password", "login", "admin", "config", "data", "temp"
        };
        
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        for (const auto& word : commonWords) {
            if (lower.find(word) != std::string::npos) {
                return true;
            }
        }
        
        // URL, 파일 경로, 레지스트리 키 패턴 확인
        if (str.find("http") != std::string::npos ||
            str.find("C:\\") != std::string::npos ||
            str.find("HKEY_") != std::string::npos) {
            return true;
        }
        
        return false;
    }
};
```

## ⚠️ 윤리적 고려사항

### 법적 경계
```cpp
/*
 * 중요: 이 코드들은 교육 목적으로만 사용되어야 합니다.
 * 
 * 합법적 사용:
 * - 자신이 소유한 소프트웨어 분석
 * - 악성코드 분석 (보안 연구)
 * - 취약점 연구 (책임감 있는 공개)
 * - 교육 및 학습
 * 
 * 불법적 사용:
 * - 타인의 소프트웨어 무단 분석
 * - 저작권 보호 우회
 * - 악성 목적의 활용
 * - 상업적 이익을 위한 남용
 */

class EthicalGuidelines {
public:
    static void DisplayWarning() {
        std::cout << "=== 윤리적 사용 가이드라인 ===" << std::endl;
        std::cout << "이 도구는 교육 및 연구 목적으로만 사용하세요." << std::endl;
        std::cout << "불법적인 활동에 사용하지 마세요." << std::endl;
        std::cout << "해당 지역의 법률을 준수하세요." << std::endl;
        std::cout << "================================" << std::endl;
    }
    
    static bool GetUserConsent() {
        std::cout << "위 가이드라인에 동의하십니까? (y/n): ";
        char response;
        std::cin >> response;
        return response == 'y' || response == 'Y';
    }
};
```

---

**🛡️ 목표: 고급 역공학 기법을 이해하고 방어적 보안 관점에서 활용할 수 있는 능력 습득**