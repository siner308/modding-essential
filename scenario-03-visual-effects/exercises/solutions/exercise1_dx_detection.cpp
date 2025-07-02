/*
 * Exercise 1: DirectX 버전 감지
 * 
 * 문제: 게임이 사용하는 DirectX 버전(9/11/12)을 자동으로 감지하는 프로그램을 작성하세요.
 * 
 * 학습 목표:
 * - 프로세스 모듈 분석
 * - DLL 로딩 상태 확인
 * - DirectX API 버전 식별
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm> // For std::transform

class DirectXDetector {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    struct DXInfo {
        std::string version;
        std::vector<std::string> loadedModules;
        bool isConfirmed;
    };

public:
    DirectXDetector() : processHandle(nullptr), processId(0) {}
    
    ~DirectXDetector() {
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }
    
    bool AttachToProcess(const std::wstring& targetProcess) {
        processName = targetProcess;
        
        // 프로세스 찾기
        if (!FindProcess()) {
            return false;
        }
        
        // 프로세스 핸들 열기
        processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들 열기 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
        
        std::wcout << L"프로세스에 연결됨: " << processName << L" (PID: " << processId << L")" << std::endl;
        return true;
    }
    
    DXInfo DetectDirectXVersion() {
        DXInfo dxInfo;
        dxInfo.isConfirmed = false;
        
        std::wcout << L"DirectX 버전 감지 중..." << std::endl;
        
        // 로드된 모듈 목록 가져오기
        std::vector<std::string> modules = GetLoadedModules();
        
        // DirectX 관련 DLL 확인
        std::map<std::string, std::vector<std::string>> dxPatterns = {
            {"DirectX 9", {"d3d9.dll", "d3dx9_*.dll"}},
            {"DirectX 10", {"d3d10.dll", "d3d10_1.dll", "dxgi.dll"}},
            {"DirectX 11", {"d3d11.dll", "dxgi.dll"}},
            {"DirectX 12", {"d3d12.dll", "dxgi.dll"}}
        };
        
        std::map<std::string, int> versionScores;
        
        // 각 버전별로 점수 계산
        for (const auto& pattern : dxPatterns) {
            int score = 0;
            std::vector<std::string> foundModules;
            
            for (const auto& dll : pattern.second) {
                for (const auto& module : modules) {
                    if (MatchPattern(module, dll)) {
                        score++;
                        foundModules.push_back(module);
                        break;
                    }
                }
            }
            
            if (score > 0) {
                versionScores[pattern.first] = score;
                if (pattern.first == "DirectX 12" && score >= 2) dxInfo.isConfirmed = true;
                if (pattern.first == "DirectX 11" && score >= 2) dxInfo.isConfirmed = true;
                if (pattern.first == "DirectX 10" && score >= 2) dxInfo.isConfirmed = true;
                if (pattern.first == "DirectX 9" && score >= 1) dxInfo.isConfirmed = true;
            }
        }
        
        // 가장 높은 점수의 버전 선택
        std::string detectedVersion = "Unknown";
        int maxScore = 0;
        
        for (const auto& score : versionScores) {
            if (score.second > maxScore) {
                maxScore = score.second;
                detectedVersion = score.first;
            }
        }
        
        dxInfo.version = detectedVersion;
        dxInfo.loadedModules = modules;
        
        // 추가 검증
        if (dxInfo.isConfirmed) {
            dxInfo.isConfirmed = VerifyDirectXVersion(detectedVersion);
        }
        
        return dxInfo;
    }
    
    std::vector<std::string> GetLoadedModules() {
        std::vector<std::string> modules;
        
        HMODULE hModules[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(processHandle, hModules, sizeof(hModules), &cbNeeded)) {
            DWORD moduleCount = cbNeeded / sizeof(HMODULE);
            
            for (DWORD i = 0; i < moduleCount; i++) {
                char moduleName[MAX_PATH];
                if (GetModuleBaseNameA(processHandle, hModules[i], moduleName, sizeof(moduleName))) {
                    modules.push_back(std::string(moduleName));
                }
            }
        }
        
        return modules;
    }
    
    bool MatchPattern(const std::string& moduleName, const std::string& pattern) {
        // 간단한 와일드카드 매칭
        if (pattern.find('*') != std::string::npos) {
            std::string prefix = pattern.substr(0, pattern.find('*'));
            std::string suffix = pattern.substr(pattern.find('*') + 1);
            
            return moduleName.find(prefix) == 0 && 
                   moduleName.find(suffix) == (moduleName.length() - suffix.length());
        } else {
            return moduleName == pattern;
        }
    }
    
    bool VerifyDirectXVersion(const std::string& version) {
        if (version == "DirectX 12") {
            return CheckD3D12Support();
        } else if (version == "DirectX 11") {
            return CheckD3D11Support();
        } else if (version == "DirectX 10") {
            return CheckD3D10Support();
        } else if (version == "DirectX 9") {
            return CheckD3D9Support();
        }
        
        return false;
    }
    
    bool CheckD3D12Support() {
        // D3D12 특정 함수 주소 확인
        HMODULE d3d12Module = GetModuleHandleA("d3d12.dll");
        if (!d3d12Module) return false;
        
        // D3D12CreateDevice 함수 존재 확인
        return GetProcAddress(d3d12Module, "D3D12CreateDevice") != nullptr;
    }
    
    bool CheckD3D11Support() {
        // D3D11 특정 함수 주소 확인
        HMODULE d3d11Module = GetModuleHandleA("d3d11.dll");
        if (!d3d11Module) return false;
        
        // D3D11CreateDevice 함수 존재 확인
        return GetProcAddress(d3d11Module, "D3D11CreateDevice") != nullptr;
    }
    
    bool CheckD3D10Support() {
        // D3D10 특정 함수 주소 확인
        HMODULE d3d10Module = GetModuleHandleA("d3d10.dll");
        if (!d3d10Module) return false;
        
        // D3D10CreateDevice 함수 존재 확인
        return GetProcAddress(d3d10Module, "D3D10CreateDevice") != nullptr;
    }
    
    bool CheckD3D9Support() {
        // D3D9 특정 함수 주소 확인
        HMODULE d3d9Module = GetModuleHandleA("d3d9.dll");
        if (!d3d9Module) return false;
        
        // Direct3DCreate9 함수 존재 확인
        return GetProcAddress(d3d9Module, "Direct3DCreate9") != nullptr;
    }
    
    void AnalyzeGraphicsCapabilities() {
        std::wcout << L"\n=== 그래픽 기능 분석 ===" << std::endl;
        
        // 시스템 정보
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        std::wcout << L"프로세서 아키텍처: ";
        switch (sysInfo.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:
                std::wcout << L"x64" << std::endl;
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                std::wcout << L"x86" << std::endl;
                break;
            default:
                std::wcout << L"기타" << std::endl;
                break;
        }
        
        std::wcout << L"프로세서 수: " << sysInfo.dwNumberOfProcessors << std::endl;
        
        // 메모리 정보
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            std::wcout << L"총 물리 메모리: " << (memStatus.ullTotalPhys / (1024 * 1024 * 1024)) << L" GB" << std::endl;
            std::wcout << L"사용 가능 메모리: " << (memStatus.ullAvailPhys / (1024 * 1024 * 1024)) << L" GB" << std::endl;
        }
        
        // DirectX 런타임 버전 확인
        CheckDirectXRuntime();
    }
    
    void CheckDirectXRuntime() {
        std::wcout << L"\n=== DirectX 런타임 확인 ===" << std::endl;
        
        // 레지스트리에서 DirectX 버전 확인
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                         "SOFTWARE\\Microsoft\\DirectX", 
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            char version[256];
            DWORD size = sizeof(version);
            DWORD type;
            
            if (RegQueryValueExA(hKey, "Version", nullptr, &type, 
                                reinterpret_cast<LPBYTE>(version), &size) == ERROR_SUCCESS) {
                std::wcout << L"설치된 DirectX 버전: " << std::wstring(version, version + size) << std::endl;
            }
            
            RegCloseKey(hKey);
        }
        
        // DirectX End-User Runtime 확인
        if (GetFileAttributesA("C:\\Windows\\System32\\d3dx9_43.dll") != INVALID_FILE_ATTRIBUTES) {
            std::wcout << L"DirectX 9.0c 런타임: 설치됨" << std::endl;
        } else {
            std::wcout << L"DirectX 9.0c 런타임: 미설치" << std::endl;
        }
    }
    
    void ShowDetectionResults(const DXInfo& dxInfo) {
        std::wcout << L"\n=== DirectX 감지 결과 ===" << std::endl;
        std::wcout << L"감지된 버전: " << std::wstring(dxInfo.version.begin(), dxInfo.version.end()) << std::endl;
        std::wcout << L"확신도: " << (dxInfo.isConfirmed ? L"높음" : L"낮음") << std::endl;
        
        std::wcout << L"\n로드된 DirectX 관련 모듈:" << std::endl;
        std::vector<std::string> dxModules;
        
        for (const auto& module : dxInfo.loadedModules) {
            if (module.find("d3d") != std::string::npos || 
                module.find("dxgi") != std::string::npos ||
                module.find("DirectX") != std::string::npos) {
                dxModules.push_back(module);
            }
        }
        
        if (dxModules.empty()) {
            std::wcout << L"  DirectX 관련 모듈을 찾을 수 없습니다." << std::endl;
        } else {
            for (const auto& module : dxModules) {
                std::wcout << L"  - " << std::wstring(module.begin(), module.end()) << std::endl;
            }
        }
        
        // 권장 후킹 방법 제시
        std::wcout << L"\n권장 후킹 방법:" << std::endl;
        if (dxInfo.version == "DirectX 12") {
            std::wcout << L"  - D3D12 Command Queue Present 후킹" << std::endl;
            std::wcout << L"  - DXGI SwapChain Present 후킹" << std::endl;
        } else if (dxInfo.version == "DirectX 11") {
            std::wcout << L"  - D3D11 Present 후킹" << std::endl;
            std::wcout << L"  - DXGI SwapChain Present 후킹" << std::endl;
        } else if (dxInfo.version == "DirectX 10") {
            std::wcout << L"  - D3D10 Present 후킹" << std::endl;
            std::wcout << L"  - DXGI SwapChain Present 후킹" << std::endl;
        } else if (dxInfo.version == "DirectX 9") {
            std::wcout << L"  - D3D9 Present/EndScene 후킹" << std::endl;
            std::wcout << L"  - D3D9 Reset 후킹 (디바이스 로스트 처리)" << std::endl;
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
            }
            while (Process32NextW(snapshot, &processEntry));
        }
        
        CloseHandle(snapshot);
        
        if (!found) {
            std::wcout << L"프로세스를 찾을 수 없습니다: " << processName << std::endl;
            return false;
        }
        
        return true;
    }
};

int main() {
    std::wcout << L"=== DirectX 버전 감지기 ===" << std::endl;
    std::wcout << L"게임이 사용하는 DirectX 버전을 자동으로 감지합니다." << std::endl;
    
    DirectXDetector detector;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: game.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 프로세스에 연결
    if (!detector.AttachToProcess(processName)) {
        std::wcout << L"프로세스 연결 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. DirectX 버전 감지" << std::endl;
        std::wcout << L"2. 로드된 모듈 목록" << std::endl;
        std::wcout << L"3. 그래픽 기능 분석" << std::endl;
        std::wcout << L"4. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1: {
                auto dxInfo = detector.DetectDirectXVersion();
                detector.ShowDetectionResults(dxInfo);
                break;
            }
            
            case 2: {
                auto modules = detector.GetLoadedModules();
                std::wcout << L"\n=== 로드된 모듈 목록 ===" << std::endl;
                std::wcout << L"총 " << modules.size() << L"개 모듈" << std::endl;
                
                for (size_t i = 0; i < modules.size(); ++i) {
                    std::wcout << L"  " << i+1 << L". " << std::wstring(modules[i].begin(), modules[i].end()) << std::endl;
                    
                    // 너무 많으면 중간에 멈춤
                    if (i > 0 && (i + 1) % 20 == 0) {
                        std::wcout << L"계속 보시겠습니까? (y/n): ";
                        wchar_t cont;
                        std::wcin >> cont;
                        if (cont != L'y' && cont != L'Y') break;
                    }
                }
                break;
            }
            
            case 3:
                detector.AnalyzeGraphicsCapabilities();
                break;
                
            case 4:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}