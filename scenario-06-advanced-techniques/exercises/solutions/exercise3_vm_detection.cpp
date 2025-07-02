#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <intrin.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>
#include <codecvt>
#include <locale>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Psapi.lib")

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

/**
 * Exercise 3: 가상 머신 탐지 시스템
 * 
 * 목표: 프로그램이 가상 머신 환경에서 실행되고 있는지 다양한 방법으로 탐지
 * 
 * 구현 내용:
 * 1. CPUID 명령어를 통한 하이퍼바이저 탐지
 * 2. 레지스트리 기반 VM 탐지
 * 3. 시스템 서비스 및 프로세스 분석
 * 4. 하드웨어 특성 분석
 * 5. 타이밍 공격을 통한 VM 탐지
 * 6. 메모리 패턴 분석
 * 7. 네트워크 어댑터 분석
 */

class VMDetector {
private:
    struct DetectionResult {
        std::string method;
        bool isVM;
        std::string details;
        int confidence; // 0-100
    };

    static std::vector<DetectionResult> results;

public:
    // 1. CPUID 기반 하이퍼바이저 탐지
    static DetectionResult DetectHypervisorCPUID() {
        DetectionResult result;
        result.method = "CPUID 하이퍼바이저 비트";
        result.isVM = false;
        result.confidence = 95;

        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        
        // ECX의 31번째 비트가 하이퍼바이저 존재를 나타냄
        bool hypervisorPresent = (cpuInfo[2] >> 31) & 1;
        
        if (hypervisorPresent) {
            result.isVM = true;
            result.details = "CPUID에서 하이퍼바이저 비트 감지됨";
            
            // 하이퍼바이저 벤더 확인
            __cpuid(cpuInfo, 0x40000000);
            char vendor[13] = {0};
            memcpy(vendor, &cpuInfo[1], 4);
            memcpy(vendor + 4, &cpuInfo[2], 4);
            memcpy(vendor + 8, &cpuInfo[3], 4);
            
            result.details += " (벤더: " + std::string(vendor) + ")";
        } else {
            result.details = "하이퍼바이저 비트 없음";
        }

        return result;
    }

    // 2. VMware 특화 탐지
    static DetectionResult DetectVMware() {
        DetectionResult result;
        result.method = "VMware 탐지";
        result.isVM = false;
        result.confidence = 90;

        std::vector<std::string> vmwareIndicators;

        // VMware 레지스트리 키 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\VMware, Inc.\\VMware Tools", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            vmwareIndicators.push_back("VMware Tools 레지스트리");
            RegCloseKey(hKey);
        }

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SYSTEM\\ControlSet001\\Services\\vmmouse", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            vmwareIndicators.push_back("VMware 마우스 드라이버");
            RegCloseKey(hKey);
        }

        // VMware 특정 프로세스 확인
        if (IsProcessRunning("vmtoolsd.exe")) {
            vmwareIndicators.push_back("VMware Tools 서비스");
        }
        if (IsProcessRunning("vmwaretray.exe")) {
            vmwareIndicators.push_back("VMware 트레이");
        }
        if (IsProcessRunning("vmwareuser.exe")) {
            vmwareIndicators.push_back("VMware 사용자 프로세스");
        }

        // VMware 디바이스 확인
        HANDLE hDevice = CreateFileA("\\\\.\\HGFS", 
                                   GENERIC_READ, FILE_SHARE_READ, 
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hDevice != INVALID_HANDLE_VALUE) {
            vmwareIndicators.push_back("VMware HGFS 디바이스");
            CloseHandle(hDevice);
        }

        // MAC 주소 확인 (VMware는 00:0C:29, 00:1C:14, 00:50:56 사용)
        if (CheckVMwareMAC()) {
            vmwareIndicators.push_back("VMware MAC 주소");
        }

        if (!vmwareIndicators.empty()) {
            result.isVM = true;
            result.details = "VMware 지표 발견: ";
            for (size_t i = 0; i < vmwareIndicators.size(); ++i) {
                if (i > 0) result.details += ", ";
                result.details += vmwareIndicators[i];
            }
        } else {
            result.details = "VMware 지표 없음";
        }

        return result;
    }

    // 3. VirtualBox 탐지
    static DetectionResult DetectVirtualBox() {
        DetectionResult result;
        result.method = "VirtualBox 탐지";
        result.isVM = false;
        result.confidence = 90;

        std::vector<std::string> vboxIndicators;

        // VirtualBox 레지스트리 키 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\Oracle\\VirtualBox Guest Additions", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            vboxIndicators.push_back("VirtualBox Guest Additions");
            RegCloseKey(hKey);
        }

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SYSTEM\\ControlSet001\\Services\\VBoxService", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            vboxIndicators.push_back("VirtualBox 서비스");
            RegCloseKey(hKey);
        }

        // VirtualBox 프로세스 확인
        if (IsProcessRunning("VBoxTray.exe")) {
            vboxIndicators.push_back("VirtualBox 트레이");
        }
        if (IsProcessRunning("VBoxService.exe")) {
            vboxIndicators.push_back("VirtualBox 서비스 프로세스");
        }

        // VirtualBox 디바이스 확인
        HANDLE hDevice = CreateFileA("\\\\.\\VBoxMiniRdrDN", 
                                   GENERIC_READ, FILE_SHARE_READ, 
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hDevice != INVALID_HANDLE_VALUE) {
            vboxIndicators.push_back("VirtualBox 미니 리다이렉터");
            CloseHandle(hDevice);
        }

        // 시스템 BIOS 확인
        if (CheckSystemManufacturer("Oracle Corporation") || 
            CheckSystemManufacturer("innotek GmbH")) {
            vboxIndicators.push_back("VirtualBox BIOS");
        }

        if (!vboxIndicators.empty()) {
            result.isVM = true;
            result.details = "VirtualBox 지표 발견: ";
            for (size_t i = 0; i < vboxIndicators.size(); ++i) {
                if (i > 0) result.details += ", ";
                result.details += vboxIndicators[i];
            }
        } else {
            result.details = "VirtualBox 지표 없음";
        }

        return result;
    }

    // 4. Hyper-V 탐지
    static DetectionResult DetectHyperV() {
        DetectionResult result;
        result.method = "Hyper-V 탐지";
        result.isVM = false;
        result.confidence = 85;

        std::vector<std::string> hypervIndicators;

        // Hyper-V 레지스트리 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            hypervIndicators.push_back("Hyper-V Guest Parameters");
            RegCloseKey(hKey);
        }

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SYSTEM\\ControlSet001\\Services\\vmbus", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            hypervIndicators.push_back("Hyper-V VMBus");
            RegCloseKey(hKey);
        }

        // Hyper-V 서비스 확인
        if (IsServiceRunning("vmicheartbeat")) {
            hypervIndicators.push_back("Hyper-V Heartbeat 서비스");
        }
        if (IsServiceRunning("vmicvss")) {
            hypervIndicators.push_back("Hyper-V VSS 서비스");
        }

        // CPUID로 Microsoft 하이퍼바이저 확인
        int cpuInfo[4];
        __cpuid(cpuInfo, 0x40000000);
        char vendor[13] = {0};
        memcpy(vendor, &cpuInfo[1], 4);
        memcpy(vendor + 4, &cpuInfo[2], 4);
        memcpy(vendor + 8, &cpuInfo[3], 4);
        
        if (strcmp(vendor, "Microsoft Hv") == 0) {
            hypervIndicators.push_back("Microsoft 하이퍼바이저 CPUID");
        }

        if (!hypervIndicators.empty()) {
            result.isVM = true;
            result.details = "Hyper-V 지표 발견: ";
            for (size_t i = 0; i < hypervIndicators.size(); ++i) {
                if (i > 0) result.details += ", ";
                result.details += hypervIndicators[i];
            }
        } else {
            result.details = "Hyper-V 지표 없음";
        }

        return result;
    }

    // 5. QEMU/KVM 탐지
    static DetectionResult DetectQEMU() {
        DetectionResult result;
        result.method = "QEMU/KVM 탐지";
        result.isVM = false;
        result.confidence = 80;

        std::vector<std::string> qemuIndicators;

        // 시스템 정보에서 QEMU 확인
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) {
            if (strstr(computerName, "QEMU") != nullptr) {
                qemuIndicators.push_back("QEMU 컴퓨터 이름");
            }
        }

        // QEMU 하드웨어 확인
        if (CheckSystemManufacturer("QEMU")) {
            qemuIndicators.push_back("QEMU 제조사");
        }

        // CPUID로 KVM 확인
        int cpuInfo[4];
        __cpuid(cpuInfo, 0x40000000);
        char vendor[13] = {0};
        memcpy(vendor, &cpuInfo[1], 4);
        memcpy(vendor + 4, &cpuInfo[2], 4);
        memcpy(vendor + 8, &cpuInfo[3], 4);
        
        if (strstr(vendor, "KVMKVMKVM") != nullptr) {
            qemuIndicators.push_back("KVM CPUID");
        }

        // QEMU 디바이스 확인
        if (CheckPCIDevices("QEMU")) {
            qemuIndicators.push_back("QEMU PCI 디바이스");
        }

        if (!qemuIndicators.empty()) {
            result.isVM = true;
            result.details = "QEMU/KVM 지표 발견: ";
            for (size_t i = 0; i < qemuIndicators.size(); ++i) {
                if (i > 0) result.details += ", ";
                result.details += qemuIndicators[i];
            }
        } else {
            result.details = "QEMU/KVM 지표 없음";
        }

        return result;
    }

    // 6. 타이밍 기반 VM 탐지
    static DetectionResult DetectVMTiming() {
        DetectionResult result;
        result.method = "타이밍 기반 탐지";
        result.isVM = false;
        result.confidence = 70;

        std::vector<DWORD> timings;
        const int iterations = 10;

        // RDTSC 명령어를 사용한 타이밍 측정
        for (int i = 0; i < iterations; ++i) {
            DWORD start = __rdtsc();
            
            // 간단한 연산 수행
            volatile int dummy = 0;
            for (int j = 0; j < 1000; ++j) {
                dummy += j;
            }
            
            DWORD end = __rdtsc();
            timings.push_back(end - start);
        }

        // 타이밍 분산 계산
        DWORD sum = 0;
        for (DWORD timing : timings) {
            sum += timing;
        }
        DWORD average = sum / iterations;

        DWORD variance = 0;
        for (DWORD timing : timings) {
            variance += (timing - average) * (timing - average);
        }
        variance /= iterations;

        // VM에서는 타이밍이 더 불안정함
        if (variance > 10000) { // 임계값은 환경에 따라 조정 필요
            result.isVM = true;
            result.details = "높은 타이밍 분산 감지 (분산: " + std::to_string(variance) + ")";
        } else {
            result.details = "정상적인 타이밍 패턴 (분산: " + std::to_string(variance) + ")";
        }

        return result;
    }

    // 7. 메모리 크기 기반 탐지
    static DetectionResult DetectVMMemory() {
        DetectionResult result;
        result.method = "메모리 기반 탐지";
        result.isVM = false;
        result.confidence = 60;

        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);

        DWORDLONG totalRAM = memStatus.ullTotalPhys / (1024 * 1024); // MB 단위

        // VM에서 흔히 사용되는 메모리 크기들
        std::vector<DWORDLONG> commonVMSizes = {512, 1024, 2048, 4096, 8192};
        
        for (DWORDLONG vmSize : commonVMSizes) {
            if (abs((long long)(totalRAM - vmSize)) < 64) { // 64MB 오차 허용
                result.isVM = true;
                result.details = "일반적인 VM 메모리 크기 (" + std::to_string(totalRAM) + "MB)";
                break;
            }
        }

        if (!result.isVM) {
            result.details = "물리 시스템 메모리 크기 (" + std::to_string(totalRAM) + "MB)";
        }

        return result;
    }

    // 8. 네트워크 어댑터 기반 탐지
    static DetectionResult DetectVMNetwork() {
        DetectionResult result;
        result.method = "네트워크 어댑터 탐지";
        result.isVM = false;
        result.confidence = 75;

        // WMI를 통해 네트워크 어댑터 정보 조회 (간단화된 버전)
        // 실제로는 WMI API를 사용해야 하지만, 여기서는 레지스트리를 통해 확인
        
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            std::vector<std::string> vmNetworkAdapters;
            
            // VM 관련 네트워크 어댑터 이름들
            std::vector<std::string> vmAdapterNames = {
                "VMware", "VirtualBox", "Hyper-V", "QEMU", "Virtual", "Ethernet"
            };

            DWORD index = 0;
            char subKeyName[256];
            DWORD subKeyNameSize = sizeof(subKeyName);

            while (RegEnumKeyExA(hKey, index++, subKeyName, &subKeyNameSize, 
                                nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                
                HKEY hSubKey;
                if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                    char driverDesc[256];
                    DWORD dataSize = sizeof(driverDesc);
                    
                    if (RegQueryValueExA(hSubKey, "DriverDesc", nullptr, nullptr, 
                                        reinterpret_cast<LPBYTE>(driverDesc), &dataSize) == ERROR_SUCCESS) {
                        
                        for (const auto& vmName : vmAdapterNames) {
                            if (strstr(driverDesc, vmName.c_str()) != nullptr) {
                                vmNetworkAdapters.push_back(driverDesc);
                                break;
                            }
                        }
                    }
                    RegCloseKey(hSubKey);
                }
                subKeyNameSize = sizeof(subKeyName);
            }
            
            RegCloseKey(hKey);
            
            if (!vmNetworkAdapters.empty()) {
                result.isVM = true;
                result.details = "VM 네트워크 어댑터 발견: ";
                for (size_t i = 0; i < vmNetworkAdapters.size(); ++i) {
                    if (i > 0) result.details += ", ";
                    result.details += vmNetworkAdapters[i];
                }
            } else {
                result.details = "물리 네트워크 어댑터만 감지됨";
            }
        }

        return result;
    }

    // 모든 탐지 방법 실행
    static std::vector<DetectionResult> RunAllDetections() {
        std::vector<DetectionResult> allResults;
        
        std::cout << "[+] VM 탐지 시작..." << std::endl;
        
        allResults.push_back(DetectHypervisorCPUID());
        allResults.push_back(DetectVMware());
        allResults.push_back(DetectVirtualBox());
        allResults.push_back(DetectHyperV());
        allResults.push_back(DetectQEMU());
        allResults.push_back(DetectVMTiming());
        allResults.push_back(DetectVMMemory());
        allResults.push_back(DetectVMNetwork());
        
        return allResults;
    }

    // 결과 분석 및 종합 판단
    static bool AnalyzeResults(const std::vector<DetectionResult>& results) {
        int totalConfidence = 0;
        int vmDetections = 0;
        
        std::cout << "\n=== VM 탐지 결과 ===" << std::endl;
        std::cout << std::left << std::setw(25) << "탐지 방법" 
                  << std::setw(10) << "결과" 
                  << std::setw(10) << "신뢰도" 
                  << "상세 정보" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::setw(25) << result.method
                      << std::setw(10) << (result.isVM ? "VM" : "물리")
                      << std::setw(10) << (std::to_string(result.confidence) + "%")
                      << result.details << std::endl;
            
            if (result.isVM) {
                totalConfidence += result.confidence;
                vmDetections++;
            }
        }
        
        std::cout << std::string(80, '-') << std::endl;
        
        // 종합 판단
        bool isVM = false;
        if (vmDetections >= 2) { // 2개 이상의 방법에서 VM 탐지
            isVM = true;
        } else if (vmDetections == 1 && totalConfidence >= 90) { // 1개지만 신뢰도 높음
            isVM = true;
        }
        
        int averageConfidence = vmDetections > 0 ? totalConfidence / vmDetections : 0;
        
        std::cout << "\n=== 종합 판단 ===" << std::endl;
        std::cout << "VM 탐지 횟수: " << vmDetections << "/" << results.size() << std::endl;
        std::cout << "평균 신뢰도: " << averageConfidence << "%" << std::endl;
        std::cout << "최종 결과: " << (isVM ? "가상 머신 환경" : "물리 시스템") << std::endl;
        
        return isVM;
    }

private:
    // 헬퍼 함수들
    static bool IsProcessRunning(const std::string& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (_stricmp(pe32.szExeFile, processName.c_str()) == 0) {
                    CloseHandle(hSnapshot);
                    return true;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
        return false;
    }

    static bool IsServiceRunning(const std::string& serviceName) {
        SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
        if (!scManager) {
            return false;
        }

        SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_QUERY_STATUS);
        if (service) {
            SERVICE_STATUS status;
            if (QueryServiceStatus(service, &status)) {
                CloseServiceHandle(service);
                CloseServiceHandle(scManager);
                return status.dwCurrentState == SERVICE_RUNNING;
            }
            CloseServiceHandle(service);
        }

        CloseHandle(scManager);
        return false;
    }

    static bool CheckSystemManufacturer(const std::string& manufacturer) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "HARDWARE\\DESCRIPTION\\System\\BIOS", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            char systemManufacturer[256];
            DWORD dataSize = sizeof(systemManufacturer);
            
            if (RegQueryValueExA(hKey, "SystemManufacturer", nullptr, nullptr, 
                                reinterpret_cast<LPBYTE>(systemManufacturer), &dataSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return strstr(systemManufacturer, manufacturer.c_str()) != nullptr;
            }
            RegCloseKey(hKey);
        }
        return false;
    }

    static bool CheckVMwareMAC() {
        // VMware MAC 주소 접두사: 00:0C:29, 00:1C:14, 00:50:56
        // 실제로는 GetAdaptersInfo API를 사용해야 하지만 간단화
        return false; // 구현 생략
    }

    static bool CheckPCIDevices(const std::string& vendor) {
        // PCI 디바이스 확인 - 실제로는 더 복잡한 구현 필요
        return false; // 구현 생략
    }
};

// Static 멤버 정의
std::vector<VMDetector::DetectionResult> VMDetector::results;

int main() {
    std::cout << "고급 가상 머신 탐지 시스템 v1.0" << std::endl;
    std::cout << "교육 및 연구 목적으로만 사용하세요." << std::endl;
    std::cout << "====================================" << std::endl;

    auto results = VMDetector::RunAllDetections();
    bool isVM = VMDetector::AnalyzeResults(results);

    if (isVM) {
        std::cout << "\n⚠️  경고: 가상 머신 환경에서 실행 중입니다!" << std::endl;
    } else {
        std::cout << "\n✅ 물리 시스템에서 실행 중입니다." << std::endl;
    }

    std::cout << "\n계속하려면 Enter를 누르세요..." << std::endl;
    std::cin.get();

    return 0;
}

/*
 * 컴파일 방법:
 * cl /EHsc exercise3_vm_detection.cpp
 * 
 * 테스트 방법:
 * 1. 물리 시스템에서 실행 - 대부분의 탐지 방법이 "물리"로 표시
 * 2. 가상 머신에서 실행 - VM 관련 지표들이 탐지됨
 * 
 * 학습 포인트:
 * - CPUID 명령어 활용
 * - 시스템 레지스트리 분석
 * - 프로세스 및 서비스 열거
 * - 하드웨어 특성 분석
 * - 타이밍 공격 기법
 * - 종합적인 휴리스틱 분석
 * 
 * 탐지 가능한 VM:
 * - VMware Workstation/ESXi
 * - Oracle VirtualBox
 * - Microsoft Hyper-V
 * - QEMU/KVM
 * - 기타 하이퍼바이저
 * 
 * 회피 기법:
 * - VM 설정 수정 (CPU 기능 마스킹)
 * - 레지스트리 정리
 * - 프로세스/서비스 숨김
 * - 하드웨어 정보 위조
 */