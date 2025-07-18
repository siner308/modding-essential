/*
 * Exercise 2: 동적 주소 추적
 * 
 * 문제: 게임 재시작 후에도 FPS 주소를 자동으로 찾는 시스템을 구현하세요.
 * 
 * 학습 목표:
 * - 포인터 체인 분석
 * - 베이스 주소 + 오프셋 패턴
 * - 시그니처 기반 주소 찾기
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <json/json.h>
#include <algorithm>
#include <cmath> // For std::isfinite

#pragma comment(lib, "jsoncpp.lib")

class DynamicAddressTracker {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    struct ModuleInfo {
        std::string name;
        uintptr_t baseAddress;
        size_t size;
        std::string version;
    };
    
    struct PointerPath {
        std::string moduleName;
        uintptr_t baseOffset;
        std::vector<uintptr_t> offsets;
        std::string description;
        bool isValid;
        uintptr_t lastResolvedAddress;
    };
    
    struct SignaturePattern {
        std::vector<uint8_t> bytes;
        std::vector<bool> mask;
        std::string name;
        int offsetToTarget;
        bool isRelativeOffset;
    };
    
    std::map<std::string, ModuleInfo> modules;
    std::vector<PointerPath> knownPaths;
    std::vector<SignaturePattern> signatures;
    std::string configFile;

public:
    DynamicAddressTracker() : processHandle(nullptr), processId(0) {
        configFile = "fps_addresses.json";
        LoadKnownPatterns();
    }
    
    ~DynamicAddressTracker() {
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
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        // 모듈 정보 수집
        if (!CollectModules()) {
            std::wcout << L"모듈 정보 수집 실패" << std::endl;
            return false;
        }
        
        // 저장된 설정 로드
        LoadConfiguration();
        
        std::wcout << L"동적 주소 추적 시스템 초기화 완료" << std::endl;
        return true;
    }
    
    void LoadKnownPatterns() {
        // Elden Ring FPS 패턴
        AddSignature("EldenRing_FPS_Pattern1", 
                    {0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0x80},
                    {true, true, true, false, false, false, false, true, true, true, true},
                    3, true);
        
        AddSignature("EldenRing_FPS_Pattern2",
                    {0xF3, 0x0F, 0x11, 0x05, 0x00, 0x00, 0x00, 0x00, 0xF3, 0x0F, 0x10, 0x05},
                    {true, true, true, true, false, false, false, false, true, true, true, true},
                    4, true);
        
        // Dark Souls III 패턴
        AddSignature("DarkSouls3_FPS_Pattern",
                    {0x89, 0x05, 0x00, 0x00, 0x00, 0x00, 0x85, 0xC0},
                    {true, true, false, false, false, false, true, true},
                    2, true);
        
        // Skyrim SE 패턴
        AddSignature("SkyrimSE_FPS_Pattern",
                    {0xF3, 0x0F, 0x2C, 0x05, 0x00, 0x00, 0x00, 0x00, 0x89, 0x05},
                    {true, true, true, true, false, false, false, false, true, true},
                    4, true);
        
        // 공통 포인터 패스 추가
        AddPointerPath("MainModule", 0x0, {0x8, 0x10, 0x18, 0x140}, "FPS 제한 값 (4단계 포인터)");
        AddPointerPath("MainModule", 0x0, {0x20, 0x30}, "FPS 제한 값 (2단계 포인터)");
    }
    
    void AddSignature(const std::string& name, const std::vector<uint8_t>& bytes, 
                     const std::vector<bool>& mask, int offset, bool relative) {
        SignaturePattern pattern;
        pattern.name = name;
        pattern.bytes = bytes;
        pattern.mask = mask;
        pattern.offsetToTarget = offset;
        pattern.isRelativeOffset = relative;
        signatures.push_back(pattern);
    }
    
    void AddPointerPath(const std::string& moduleName, uintptr_t baseOffset, 
                       const std::vector<uintptr_t>& offsets, const std::string& description) {
        PointerPath path;
        path.moduleName = moduleName;
        path.baseOffset = baseOffset;
        path.offsets = offsets;
        path.description = description;
        path.isValid = false;
        path.lastResolvedAddress = 0;
        knownPaths.push_back(path);
    }
    
    std::vector<uintptr_t> FindFPSAddresses() {
        std::vector<uintptr_t> foundAddresses;
        
        std::wcout << L"FPS 주소 자동 탐지 시작..." << std::endl;
        
        // 1. 시그니처 기반 검색
        std::wcout << L"1. 시그니처 패턴 검색..." << std::endl;
        auto signatureResults = ScanSignatures();
        foundAddresses.insert(foundAddresses.end(), signatureResults.begin(), signatureResults.end());
        
        // 2. 포인터 패스 검색
        std::wcout << L"2. 포인터 패스 검색..." << std::endl;
        auto pointerResults = ScanPointerPaths();
        foundAddresses.insert(foundAddresses.end(), pointerResults.begin(), pointerResults.end());
        
        // 3. 저장된 주소 검증
        std::wcout << L"3. 저장된 주소 검증..." << std::endl;
        auto savedResults = ValidateSavedAddresses();
        foundAddresses.insert(foundAddresses.end(), savedResults.begin(), savedResults.end());
        
        // 중복 제거
        std::sort(foundAddresses.begin(), foundAddresses.end());
        foundAddresses.erase(std::unique(foundAddresses.begin(), foundAddresses.end()), foundAddresses.end());
        
        std::wcout << L"총 " << foundAddresses.size() << L"개의 후보 주소 발견" << std::endl;
        return foundAddresses;
    }
    
    std::vector<uintptr_t> ScanSignatures() {
        std::vector<uintptr_t> results;
        
        for (const auto& sig : signatures) {
            std::wcout << L"  패턴 검색: " << std::wstring(sig.name.begin(), sig.name.end()) << std::endl;
            
            for (const auto& module : modules) {
                auto addresses = ScanModuleForSignature(module.second, sig);
                for (uintptr_t addr : addresses) {
                    uintptr_t targetAddr = ResolveSignatureAddress(addr, sig);
                    if (targetAddr != 0 && IsValidFPSAddress(targetAddr)) {
                        results.push_back(targetAddr);
                        std::wcout << L"    발견: 0x" << std::hex << targetAddr << std::dec << std::endl;
                    }
                }
            }
        }
        
        return results;
    }
    
    std::vector<uintptr_t> ScanModuleForSignature(const ModuleInfo& module, const SignaturePattern& signature) {
        std::vector<uintptr_t> results;
        
        const size_t chunkSize = 1024 * 1024; // 1MB씩 스캔
        std::vector<uint8_t> buffer(chunkSize);
        
        for (size_t offset = 0; offset < module.size; offset += chunkSize) {
            size_t readSize = std::min(chunkSize, module.size - offset);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(module.baseAddress + offset),
                                 buffer.data(), readSize, &bytesRead)) {
                
                for (size_t i = 0; i <= bytesRead - signature.bytes.size(); ++i) {
                    if (MatchesSignature(buffer.data() + i, signature)) {
                        results.push_back(module.baseAddress + offset + i);
                    }
                }
            }
        }
        
        return results;
    }
    
    bool MatchesSignature(const uint8_t* data, const SignaturePattern& signature) {
        for (size_t i = 0; i < signature.bytes.size(); ++i) {
            if (signature.mask[i] && data[i] != signature.bytes[i]) {
                return false;
            }
        }
        return true;
    }
    
    uintptr_t ResolveSignatureAddress(uintptr_t signatureAddress, const SignaturePattern& signature) {
        if (signature.isRelativeOffset) {
            // RIP 상대 주소 계산
            uint32_t offset;
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, 
                                 reinterpret_cast<LPCVOID>(signatureAddress + signature.offsetToTarget),
                                 &offset, sizeof(offset), &bytesRead) && bytesRead == sizeof(offset)) {
                
                // RIP = 명령어 끝 주소 + 오프셋
                uintptr_t instructionEnd = signatureAddress + signature.bytes.size();
                return instructionEnd + static_cast<int32_t>(offset);
            }
        } else {
            // 절대 주소
            return signatureAddress + signature.offsetToTarget;
        }
        
        return 0;
    }
    
    std::vector<uintptr_t> ScanPointerPaths() {
        std::vector<uintptr_t> results;
        
        for (auto& path : knownPaths) {
            auto moduleIt = modules.find(path.moduleName);
            if (moduleIt == modules.end()) {
                continue;
            }
            
            uintptr_t currentAddress = moduleIt->second.baseAddress + path.baseOffset;
            bool pathValid = true;
            
            // 포인터 체인 따라가기
            for (size_t i = 0; i < path.offsets.size(); ++i) {
                uintptr_t nextAddress;
                SIZE_T bytesRead;
                
                if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(currentAddress + path.offsets[i]),
                                      &nextAddress, sizeof(nextAddress), &bytesRead) || 
                    bytesRead != sizeof(nextAddress) || nextAddress == 0) {
                    pathValid = false;
                    break;
                }
                
                currentAddress = nextAddress;
            }
            
            if (pathValid && IsValidFPSAddress(currentAddress)) {
                path.isValid = true;
                path.lastResolvedAddress = currentAddress;
                results.push_back(currentAddress);
                
                std::wcout << L"  포인터 패스 성공: " << std::wstring(path.description.begin(), path.description.end())
                           << L" -> 0x" << std::hex << currentAddress << std::dec << std::endl;
            }
        }
        
        return results;
    }
    
    std::vector<uintptr_t> ValidateSavedAddresses() {
        std::vector<uintptr_t> results;
        
        // 이전에 저장된 주소들이 여전히 유효한지 확인
        Json::Value root;
        std::ifstream configStream(configFile);
        
        if (configStream.is_open()) {
            configStream >> root;
            configStream.close();
            
            if (root.isMember("savedAddresses")) {
                const Json::Value& addresses = root["savedAddresses"];
                
                for (const auto& addr : addresses) {
                    uintptr_t address = std::stoull(addr.asString(), nullptr, 16);
                    
                    if (IsValidFPSAddress(address)) {
                        results.push_back(address);
                        std::wcout << L"  저장된 주소 유효: 0x" << std::hex << address << std::dec << std::endl;
                    }
                }
            }
        }
        
        return results;
    }
    
    bool IsValidFPSAddress(uintptr_t address) {
        float value;
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                              &value, sizeof(value), &bytesRead) || bytesRead != sizeof(value)) {
            return false;
        }
        
        // 일반적인 FPS 값 범위 확인
        return value >= 10.0f && value <= 1000.0f && std::isfinite(value);
    }
    
    void SaveSuccessfulAddress(uintptr_t address, const std::string& method) {
        Json::Value root;
        std::ifstream configStream(configFile);
        
        if (configStream.is_open()) {
            configStream >> root;
            configStream.close();
        }
        
        // 새 주소 추가
        std::stringstream ss;
        ss << std::hex << address;
        
        root["savedAddresses"].append(ss.str());
        root["lastMethod"] = method;
        root["timestamp"] = static_cast<int64_t>(time(nullptr));
        
        // 모듈 정보 저장
        for (const auto& module : modules) {
            Json::Value moduleJson;
            moduleJson["baseAddress"] = std::to_string(module.second.baseAddress);
            moduleJson["size"] = std::to_string(module.second.size);
            moduleJson["version"] = module.second.version;
            
            root["modules"][module.first] = moduleJson;
        }
        
        // 파일에 저장
        std::ofstream configOut(configFile);
        if (configOut.is_open()) {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "  ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(root, &configOut);
            configOut.close();
            
            std::wcout << L"주소 정보가 저장되었습니다: " << std::wstring(configFile.begin(), configFile.end()) << std::endl;
        }
    }
    
    void CreateAddressHeuristics() {
        std::wcout << L"\n주소 패턴 학습 시작..." << std::endl;
        
        // 현재 게임에 특화된 패턴 생성
        auto modules = GetMainExecutableModules();
        
        for (const auto& module : modules) {
            std::wcout << L"모듈 분석: " << std::wstring(module.name.begin(), module.name.end()) << std::endl;
            
            // 일반적인 FPS 값들을 스캔해서 패턴 찾기
            std::vector<float> commonFPS = {30.0f, 60.0f, 120.0f, 144.0f};
            
            for (float fps : commonFPS) {
                auto addresses = ScanForFloat(module, fps);
                
                for (uintptr_t addr : addresses) {
                    AnalyzeAddressContext(addr);
                }
            }
        }
    }
    
    std::vector<ModuleInfo> GetMainExecutableModules() {
        std::vector<ModuleInfo> execModules;
        
        for (const auto& module : modules) {
            if (module.second.name.find(".exe") != std::string::npos ||
                module.second.name.find("engine") != std::string::npos ||
                module.second.name.find("game") != std::string::npos) {
                execModules.push_back(module.second);
            }
        }
        
        return execModules;
    }
    
    std::vector<uintptr_t> ScanForFloat(const ModuleInfo& module, float targetValue) {
        std::vector<uintptr_t> results;
        
        const size_t chunkSize = 1024 * 1024;
        std::vector<uint8_t> buffer(chunkSize);
        
        for (size_t offset = 0; offset < module.size; offset += chunkSize) {
            size_t readSize = std::min(chunkSize, module.size - offset);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(module.baseAddress + offset),
                                 buffer.data(), readSize, &bytesRead)) {
                
                for (size_t i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
                    float value = *reinterpret_cast<const float*>(&buffer[i]);
                    
                    if (abs(value - targetValue) < 0.01f) {
                        results.push_back(module.baseAddress + offset + i);
                    }
                }
            }
        }
        
        return results;
    }
    
    void AnalyzeAddressContext(uintptr_t address) {
        // 주소 주변의 코드 패턴 분석
        std::vector<uint8_t> context(32);
        SIZE_T bytesRead;
        
        // 주소 앞뒤 16바이트씩 읽기
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address - 16),
                             context.data(), 32, &bytesRead)) {
            
            // 공통 패턴 찾기 (예: mov, cmp, test 명령어)
            for (size_t i = 0; i < bytesRead - 4; ++i) {
                if (context[i] == 0x89 && context[i+1] == 0x05) { // mov [addr], eax
                    std::wcout << L"  MOV 패턴 발견: 0x" << std::hex << (address - 16 + i) << std::dec << std::endl;
                }
                if (context[i] == 0xF3 && context[i+1] == 0x0F && context[i+2] == 0x11) { // movss
                    std::wcout << L"  MOVSS 패턴 발견: 0x" << std::hex << (address - 16 + i) << std::dec << std::endl;
                }
            }
        }
    }
    
    void ShowTrackingResults() {
        std::wcout << L"\n=== 동적 주소 추적 결과 ===" << std::endl;
        
        std::wcout << L"\n모듈 정보:" << std::endl;
        for (const auto& module : modules) {
            std::wcout << L"  " << std::wstring(module.second.name.begin(), module.second.name.end())
                       << L": 0x" << std::hex << module.second.baseAddress << std::dec
                       << L" (크기: " << (module.second.size / 1024 / 1024) << L"MB)" << std::endl;
        }
        
        std::wcout << L"\n포인터 패스 상태:" << std::endl;
        for (const auto& path : knownPaths) {
            std::wcout << L"  " << std::wstring(path.description.begin(), path.description.end())
                       << L": " << (path.isValid ? L"유효" : L"무효");
            
            if (path.isValid) {
                std::wcout << L" (0x" << std::hex << path.lastResolvedAddress << std::dec << L")";
            }
            std::wcout << std::endl;
        }
        
        std::wcout << L"\n시그니처 패턴 수: " << signatures.size() << std::endl;
    }
    
private:
    bool FindProcess() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
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
            }
            while (Process32NextW(snapshot, &processEntry));
        }
        
        CloseHandle(snapshot);
        return found;
    }
    
    bool CollectModules() {
        modules.clear();
        
        HMODULE hModules[1024];
        DWORD cbNeeded;
        
        if (!EnumProcessModules(processHandle, hModules, sizeof(hModules), &cbNeeded)) {
            return false;
        }
        
        DWORD moduleCount = cbNeeded / sizeof(HMODULE);
        
        for (DWORD i = 0; i < moduleCount; ++i) {
            MODULEINFO modInfo;
            char moduleName[MAX_PATH];
            
            if (GetModuleInformation(processHandle, hModules[i], &modInfo, sizeof(modInfo)) &&
                GetModuleBaseNameA(processHandle, hModules[i], moduleName, sizeof(moduleName))) {
                
                ModuleInfo info;
                info.name = moduleName;
                info.baseAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
                info.size = modInfo.SizeOfImage;
                info.version = GetModuleVersion(hModules[i]);
                
                modules[info.name] = info;
            }
        }
        
        return !modules.empty();
    }
    
    std::string GetModuleVersion(HMODULE hModule) {
        char path[MAX_PATH];
        if (GetModuleFileNameA(hModule, path, sizeof(path))) {
            DWORD versionSize = GetFileVersionInfoSizeA(path, nullptr);
            if (versionSize > 0) {
                std::vector<uint8_t> versionData(versionSize);
                if (GetFileVersionInfoA(path, 0, versionSize, versionData.data())) {
                    VS_FIXEDFILEINFO* fileInfo;
                    UINT len;
                    if (VerQueryValueA(versionData.data(), "\\", 
                                      reinterpret_cast<LPVOID*>(&fileInfo), &len)) {
                        return std::to_string(HIWORD(fileInfo->dwFileVersionMS)) + "." +
                               std::to_string(LOWORD(fileInfo->dwFileVersionMS)) + "." +
                               std::to_string(HIWORD(fileInfo->dwFileVersionLS)) + "." +
                               std::to_string(LOWORD(fileInfo->dwFileVersionLS));
                    }
                }
            }
        }
        return "Unknown";
    }
    
    void LoadConfiguration() {
        std::ifstream configStream(configFile);
        if (!configStream.is_open()) {
            return;
        }
        
        Json::Value root;
        configStream >> root;
        configStream.close();
        
        // 저장된 포인터 패스 로드
        if (root.isMember("customPaths")) {
            const Json::Value& paths = root["customPaths"];
            
            for (const auto& pathJson : paths) {
                PointerPath path;
                path.moduleName = pathJson["module"].asString();
                path.baseOffset = std::stoull(pathJson["baseOffset"].asString(), nullptr, 16);
                path.description = pathJson["description"].asString();
                
                for (const auto& offset : pathJson["offsets"]) {
                    path.offsets.push_back(std::stoull(offset.asString(), nullptr, 16));
                }
                
                knownPaths.push_back(path);
            }
        }
    }
};

int main() {
    std::wcout << L"=== 동적 FPS 주소 추적 시스템 ===" << std::endl;
    std::wcout << L"게임 재시작 후에도 FPS 주소를 자동으로 찾습니다." << std::endl;
    
    DynamicAddressTracker tracker;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 시스템 초기화
    if (!tracker.Initialize(processName)) {
        std::wcout << L"시스템 초기화 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. FPS 주소 자동 탐지" << std::endl;
        std::wcout << L"2. 추적 결과 보기" << std::endl;
        std::wcout << L"3. 주소 패턴 학습" << std::endl;
        std::wcout << L"4. 성공한 주소 저장" << std::endl;
        std::wcout << L"5. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1: {
                auto addresses = tracker.FindFPSAddresses();
                
                if (!addresses.empty()) {
                    std::wcout << L"\n발견된 FPS 주소:" << std::endl;
                    for (size_t i = 0; i < addresses.size(); ++i) {
                        std::wcout << L"  " << i+1 << L". 0x" << std::hex << addresses[i] << std::dec << std::endl;
                    }
                } else {
                    std::wcout << L"FPS 주소를 찾지 못했습니다." << std::endl;
                }
                break;
            }
            
            case 2:
                tracker.ShowTrackingResults();
                break;
                
            case 3:
                tracker.CreateAddressHeuristics();
                break;
                
            case 4: {
                std::wcout << L"저장할 주소를 입력하세요 (16진수): 0x";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                
                tracker.SaveSuccessfulAddress(address, "Manual");
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