/*
 * Exercise 3: 안전한 FPS 변경
 * 
 * 문제: 게임별 안전한 FPS 범위를 확인하고 제한하는 기능을 만드세요.
 * 
 * 학습 목표:
 * - 게임 엔진별 특성 이해
 * - 안전 범위 검증
 * - 점진적 FPS 변경
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>
#include <cmath> // For std::abs

class SafeFPSChanger {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    struct GameProfile {
        std::string name;
        std::string engine;
        float minSafeFPS;
        float maxSafeFPS;
        float defaultFPS;
        std::vector<float> recommendedFPS;
        std::vector<std::string> warnings;
        bool requiresGradualChange;
        int changeDelay; // milliseconds
        bool hasPhysicsTied;
        std::string notes;
    };
    
    struct FPSChangeRequest {
        uintptr_t address;
        float currentFPS;
        float targetFPS;
        float step;
        bool isGradual;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point lastChange;
    };
    
    std::map<std::string, GameProfile> gameProfiles;
    std::vector<FPSChangeRequest> activeRequests;
    GameProfile currentProfile;
    bool profileLoaded;

public:
    SafeFPSChanger() : processHandle(nullptr), processId(0), profileLoaded(false) {
        LoadGameProfiles();
    }
    
    ~SafeFPSChanger() {
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
        
        // 게임 프로필 로드
        LoadGameProfile();
        
        std::wcout << L"안전한 FPS 변경 시스템 초기화 완료" << std::endl;
        return true;
    }
    
    void LoadGameProfiles() {
        // Elden Ring / FromSoftware 게임
        GameProfile eldenRing;
        eldenRing.name = "Elden Ring";
        eldenRing.engine = "FromSoftware Engine";
        eldenRing.minSafeFPS = 30.0f;
        eldenRing.maxSafeFPS = 165.0f;
        eldenRing.defaultFPS = 60.0f;
        eldenRing.recommendedFPS = {60.0f, 120.0f, 144.0f, 165.0f};
        eldenRing.warnings = {
            "120 FPS 초과 시 일부 애니메이션 문제 가능",
            "165 FPS 초과 시 게임 로직 오류 발생 가능",
            "온라인 플레이 시 높은 FPS는 문제가 될 수 있음"
        };
        eldenRing.requiresGradualChange = true;
        eldenRing.changeDelay = 100;
        eldenRing.hasPhysicsTied = true;
        eldenRing.notes = "물리 연산이 프레임레이트에 연동되어 있어 주의 필요";
        gameProfiles["eldenring.exe"] = eldenRing;
        
        // Dark Souls III
        GameProfile darkSouls3 = eldenRing;
        darkSouls3.name = "Dark Souls III";
        darkSouls3.maxSafeFPS = 120.0f;
        darkSouls3.recommendedFPS = {60.0f, 90.0f, 120.0f};
        gameProfiles["darksoulsiii.exe"] = darkSouls3;
        
        // Skyrim Special Edition
        GameProfile skyrimSE;
        skyrimSE.name = "Skyrim Special Edition";
        skyrimSE.engine = "Creation Engine";
        skyrimSE.minSafeFPS = 30.0f;
        skyrimSE.maxSafeFPS = 144.0f;
        skyrimSE.defaultFPS = 60.0f;
        skyrimSE.recommendedFPS = {60.0f, 72.0f, 90.0f, 120.0f, 144.0f};
        skyrimSE.warnings = {
            "120 FPS 초과 시 물리 오브젝트 이상 동작",
            "144 FPS 초과 시 Havok 물리 엔진 불안정",
            "높은 FPS에서 NPC 대화 동기화 문제 가능"
        };
        skyrimSE.requiresGradualChange = false;
        skyrimSE.changeDelay = 0;
        skyrimSE.hasPhysicsTied = true;
        skyrimSE.notes = "Havok 물리 엔진 연동으로 인한 제한";
        gameProfiles["skyrimse.exe"] = skyrimSE;
        
        // The Witcher 3
        GameProfile witcher3;
        witcher3.name = "The Witcher 3";
        witcher3.engine = "REDengine";
        witcher3.minSafeFPS = 30.0f;
        witcher3.maxSafeFPS = 300.0f;
        witcher3.defaultFPS = 60.0f;
        witcher3.recommendedFPS = {60.0f, 120.0f, 144.0f, 240.0f};
        witcher3.warnings = {
            "240 FPS 초과 시 일부 이펙트 문제 가능"
        };
        witcher3.requiresGradualChange = false;
        witcher3.changeDelay = 0;
        witcher3.hasPhysicsTied = false;
        witcher3.notes = "비교적 안정적인 고프레임 지원";
        gameProfiles["witcher3.exe"] = witcher3;
        
        // 기본 프로필
        GameProfile defaultProfile;
        defaultProfile.name = "Unknown Game";
        defaultProfile.engine = "Unknown";
        defaultProfile.minSafeFPS = 30.0f;
        defaultProfile.maxSafeFPS = 120.0f;
        defaultProfile.defaultFPS = 60.0f;
        defaultProfile.recommendedFPS = {60.0f, 90.0f, 120.0f};
        defaultProfile.warnings = {
            "알 수 없는 게임이므로 보수적인 범위 적용",
            "높은 FPS 설정 시 주의 필요"
        };
        defaultProfile.requiresGradualChange = true;
        defaultProfile.changeDelay = 200;
        defaultProfile.hasPhysicsTied = true;
        defaultProfile.notes = "보수적 기본 설정";
        gameProfiles["default"] = defaultProfile;
    }
    
    void LoadGameProfile() {
        std::string processNameStr(processName.begin(), processName.end());
        std::transform(processNameStr.begin(), processNameStr.end(), processNameStr.begin(), ::tolower);
        
        auto it = gameProfiles.find(processNameStr);
        if (it != gameProfiles.end()) {
            currentProfile = it->second;
            profileLoaded = true;
            std::wcout << L"게임 프로필 로드됨: " << std::wstring(currentProfile.name.begin(), currentProfile.name.end()) << std::endl;
        } else {
            currentProfile = gameProfiles["default"];
            profileLoaded = true;
            std::wcout << L"기본 프로필 적용됨" << std::endl;
        }
        
        ShowGameProfile();
    }
    
    void ShowGameProfile() {
        std::wcout << L"\n=== 게임 프로필 정보 ===" << std::endl;
        std::wcout << L"게임: " << std::wstring(currentProfile.name.begin(), currentProfile.name.end()) << std::endl;
        std::wcout << L"엔진: " << std::wstring(currentProfile.engine.begin(), currentProfile.engine.end()) << std::endl;
        std::wcout << L"안전 범위: " << currentProfile.minSafeFPS << L" - " << currentProfile.maxSafeFPS << L" FPS" << std::endl;
        std::wcout << L"기본 FPS: " << currentProfile.defaultFPS << std::endl;
        
        std::wcout << L"권장 FPS: ";
        for (size_t i = 0; i < currentProfile.recommendedFPS.size(); ++i) {
            std::wcout << currentProfile.recommendedFPS[i];
            if (i < currentProfile.recommendedFPS.size() - 1) std::wcout << L", ";
        }
        std::wcout << std::endl;
        
        std::wcout << L"물리 연동: " << (currentProfile.hasPhysicsTied ? L"예" : L"아니오") << std::endl;
        std::wcout << L"점진적 변경: " << (currentProfile.requiresGradualChange ? L"필요" : L"불필요") << std::endl;
        
        if (!currentProfile.warnings.empty()) {
            std::wcout << L"\n⚠️ 주의사항:" << std::endl;
            for (const auto& warning : currentProfile.warnings) {
                std::wcout << L"  - " << std::wstring(warning.begin(), warning.end()) << std::endl;
            }
        }
        
        std::wcout << L"\n참고: " << std::wstring(currentProfile.notes.begin(), currentProfile.notes.end()) << std::endl;
    }
    
    bool ValidateFPSChange(float currentFPS, float targetFPS, std::string& errorMessage) {
        if (!profileLoaded) {
            errorMessage = "게임 프로필이 로드되지 않았습니다";
            return false;
        }
        
        // 기본 범위 검증
        if (targetFPS < currentProfile.minSafeFPS) {
            errorMessage = "목표 FPS가 최소 안전 범위보다 낮습니다 (최소: " +
                          std::to_string(currentProfile.minSafeFPS) + ")";
            return false;
        }
        
        if (targetFPS > currentProfile.maxSafeFPS) {
            errorMessage = "목표 FPS가 최대 안전 범위를 초과합니다 (최대: " +
                          std::to_string(currentProfile.maxSafeFPS) + ")";
            return false;
        }
        
        // 급격한 변화 검증
        float change = std::abs(targetFPS - currentFPS);
        if (currentProfile.requiresGradualChange && change > 60.0f) {
            errorMessage = "이 게임은 점진적 FPS 변경이 필요합니다 (최대 변화: 60 FPS)";
            return false;
        }
        
        // 물리 연동 게임 특별 검증
        if (currentProfile.hasPhysicsTied) {
            // 특정 위험 구간 확인
            if (targetFPS > 120.0f && currentProfile.name.find("Souls") != std::string::npos) {
                errorMessage = "경고: 120 FPS 초과 시 게임 로직 문제가 발생할 수 있습니다";
                // 경고이지만 허용
            }
            
            if (targetFPS > 144.0f && currentProfile.engine.find("Creation") != std::string::npos) {
                errorMessage = "Creation Engine에서 144 FPS 초과는 권장되지 않습니다";
                return false;
            }
        }
        
        return true;
    }
    
    bool SafeChangeFPS(uintptr_t address, float targetFPS) {
        // 현재 FPS 값 읽기
        float currentFPS;
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                              &currentFPS, sizeof(currentFPS), &bytesRead) || 
            bytesRead != sizeof(currentFPS)) {
            std::wcout << L"현재 FPS 값을 읽을 수 없습니다" << std::endl;
            return false;
        }
        
        // 변경 검증
        std::string errorMessage;
        if (!ValidateFPSChange(currentFPS, targetFPS, errorMessage)) {
            std::wcout << L"FPS 변경 실패: " << std::wstring(errorMessage.begin(), errorMessage.end()) << std::endl;
            return false;
        }
        
        std::wcout << L"FPS 변경: " << currentFPS << L" -> " << targetFPS << std::endl;
        
        // 점진적 변경이 필요한 경우
        if (currentProfile.requiresGradualChange && std::abs(targetFPS - currentFPS) > 30.0f) {
            return GradualChangeFPS(address, currentFPS, targetFPS);
        } else {
            return DirectChangeFPS(address, targetFPS);
        }
    }
    
    bool DirectChangeFPS(uintptr_t address, float targetFPS) {
        SIZE_T bytesWritten;
        
        if (WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address),
                              &targetFPS, sizeof(targetFPS), &bytesWritten) && 
            bytesWritten == sizeof(targetFPS)) {
            
            std::wcout << L"FPS 변경 완료: " << targetFPS << std::endl;
            
            // 변경 후 안정성 검증
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return VerifyFPSChange(address, targetFPS);
        } else {
            std::wcout << L"FPS 변경 실패. 오류: " << GetLastError() << std::endl;
            return false;
        }
    }
    
    bool GradualChangeFPS(uintptr_t address, float currentFPS, float targetFPS) {
        std::wcout << L"점진적 FPS 변경 시작..." << std::endl;
        
        FPSChangeRequest request;
        request.address = address;
        request.currentFPS = currentFPS;
        request.targetFPS = targetFPS;
        request.isGradual = true;
        request.startTime = std::chrono::system_clock::now();
        request.lastChange = request.startTime;
        
        // 변경 스텝 계산
        float totalChange = targetFPS - currentFPS;
        int steps = static_cast<int>(std::abs(totalChange) / 10.0f) + 1; // 10 FPS씩 변경
        request.step = totalChange / steps;
        
        activeRequests.push_back(request);
        
        // 점진적 변경 실행
        float currentValue = currentFPS;
        
        for (int i = 1; i <= steps; ++i) {
            float nextValue = currentFPS + (request.step * i);
            
            // 마지막 스텝은 정확한 목표값
            if (i == steps) {
                nextValue = targetFPS;
            }
            
            std::wcout << L"  단계 " << i << L"/" << steps << L": " << nextValue << L" FPS" << std::endl;
            
            if (!DirectChangeFPS(address, nextValue)) {
                std::wcout << L"점진적 변경 실패" << std::endl;
                return false;
            }
            
            // 게임별 지연 시간
            if (currentProfile.changeDelay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(currentProfile.changeDelay));
            }
            
            currentValue = nextValue;
        }
        
        std::wcout << L"점진적 FPS 변경 완료" << std::endl;
        return true;
    }
    
    bool VerifyFPSChange(uintptr_t address, float expectedFPS) {
        float actualFPS;
        SIZE_T bytesRead;
        
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                             &actualFPS, sizeof(actualFPS), &bytesRead) && 
            bytesRead == sizeof(actualFPS)) {
            
            if (std::abs(actualFPS - expectedFPS) < 0.1f) {
                std::wcout << L"✓ FPS 변경 검증 성공: " << actualFPS << std::endl;
                return true;
            } else {
                std::wcout << L"✗ FPS 변경 검증 실패: 예상 " << expectedFPS 
                           << L", 실제 " << actualFPS << std::endl;
                return false;
            }
        }
        
        std::wcout << L"✗ FPS 값 읽기 실패" << std::endl;
        return false;
    }
    
    void ShowRecommendedFPS() {
        std::wcout << L"\n=== 권장 FPS 설정 ===" << std::endl;
        
        for (size_t i = 0; i < currentProfile.recommendedFPS.size(); ++i) {
            float fps = currentProfile.recommendedFPS[i];
            std::wcout << L"  " << i+1 << L". " << fps << L" FPS";
            
            if (fps == currentProfile.defaultFPS) {
                std::wcout << L" (기본값)";
            }
            
            // 각 FPS별 설명
            if (fps == 60.0f) {
                std::wcout << L" - 표준 게임 환경";
            } else if (fps == 120.0f) {
                std::wcout << L" - 고주사율 모니터 권장";
            } else if (fps == 144.0f) {
                std::wcout << L" - 144Hz 모니터용";
            } else if (fps == 165.0f) {
                std::wcout << L" - 165Hz 모니터용 (주의 필요)";
            }
            
            std::wcout << L" (240 FPS 초과 시 일부 이펙트 문제 가능)";
            
            std::wcout << std::endl;
        }
        
        if (currentProfile.hasPhysicsTied) {
            std::wcout << L"\n⚠️ 물리 연동 게임입니다. 높은 FPS 설정 시 주의하세요." << std::endl;
        }
    }
    
    void TestFPSStability(uintptr_t address, float testFPS, int duration = 10) {
        std::wcout << L"\nFPS 안정성 테스트 시작 (" << testFPS << L" FPS, " << duration << L"초)" << std::endl;
        
        // 현재 FPS 백업
        float originalFPS;
        SIZE_T bytesRead;
        ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                         &originalFPS, sizeof(originalFPS), &bytesRead);
        
        // 테스트 FPS 적용
        if (!SafeChangeFPS(address, testFPS)) {
            std::wcout << L"테스트 FPS 적용 실패" << std::endl;
            return;
        }
        
        // 안정성 모니터링
        auto startTime = std::chrono::system_clock::now();
        auto endTime = startTime + std::chrono::seconds(duration);
        
        std::vector<float> readings;
        int errorCount = 0;
        
        while (std::chrono::system_clock::now() < endTime) {
            float currentFPS;
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                                 &currentFPS, sizeof(currentFPS), &bytesRead) && 
                bytesRead == sizeof(currentFPS)) {
                
                readings.push_back(currentFPS);
                
                if (std::abs(currentFPS - testFPS) > 1.0f) {
                    errorCount++;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::wcout << L"\r테스트 중... " << readings.size() << L"번째 측정";
        }
        
        std::wcout << std::endl;
        
        // 결과 분석
        if (!readings.empty()) {
            float minFPS = *std::min_element(readings.begin(), readings.end());
            float maxFPS = *std::max_element(readings.begin(), readings.end());
            float avgFPS = 0;
            for (float fps : readings) avgFPS += fps;
            avgFPS /= readings.size();
            
            std::wcout << L"\n=== 안정성 테스트 결과 ===" << std::endl;
            std::wcout << L"측정 횟수: " << readings.size() << std::endl;
            std::wcout << L"평균 FPS: " << std::fixed << std::setprecision(1) << avgFPS << std::endl;
            std::wcout << L"최소/최대: " << minFPS << L" / " << maxFPS << std::endl;
            std::wcout << L"오류 횟수: " << errorCount << L" (" 
                       << (errorCount * 100.0f / readings.size()) << L"%)" << std::endl;
            
            if (errorCount == 0) {
                std::wcout << L"✓ 안정적: 이 FPS 설정은 안전합니다" << std::endl;
            } else if (errorCount < readings.size() * 0.1f) {
                std::wcout << L"⚠️ 주의: 가끔 불안정하지만 사용 가능합니다" << std::endl;
            } else {
                std::wcout << L"✗ 불안정: 이 FPS 설정은 권장되지 않습니다" << std::endl;
            }
        }
        
        // 원본 FPS 복원
        std::wcout << L"\n원본 FPS 복원 중..." << std::endl;
        DirectChangeFPS(address, originalFPS);
    }
    
    void CreateCustomProfile() {
        std::wcout << L"\n=== 커스텀 프로필 생성 ===" << std::endl;
        
        GameProfile customProfile = currentProfile;
        
        std::wcout << L"최소 안전 FPS (현재: " << customProfile.minSafeFPS << L"): ";
        float minFPS;
        std::wcin >> minFPS;
        customProfile.minSafeFPS = minFPS;
        
        std::wcout << L"최대 안전 FPS (현재: " << customProfile.maxSafeFPS << L"): ";
        float maxFPS;
        std::wcin >> maxFPS;
        customProfile.maxSafeFPS = maxFPS;
        
        std::wcout << L"점진적 변경 필요 여부 (1=예, 0=아니오): ";
        int gradual;
        std::wcin >> gradual;
        customProfile.requiresGradualChange = (gradual == 1);
        
        std::wcout << L"변경 지연 시간 (ms): ";
        std::wcin >> customProfile.changeDelay;
        
        // 프로필 저장
        std::string processNameStr(processName.begin(), processName.end());
        gameProfiles[processNameStr] = customProfile;
        currentProfile = customProfile;
        
        // 파일에 저장
        SaveCustomProfile(customProfile);
        
        std::wcout << L"커스텀 프로필이 생성되고 적용되었습니다." << std::endl;
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
    
    void SaveCustomProfile(const GameProfile& profile) {
        std::string filename = "custom_profile_" + std::string(processName.begin(), processName.end()) + ".txt";
        std::ofstream file(filename);
        
        if (file.is_open()) {
            file << "Game: " << profile.name << std::endl;
            file << "Engine: " << profile.engine << std::endl;
            file << "MinSafeFPS: " << profile.minSafeFPS << std::endl;
            file << "MaxSafeFPS: " << profile.maxSafeFPS << std::endl;
            file << "RequiresGradualChange: " << profile.requiresGradualChange << std::endl;
            file << "ChangeDelay: " << profile.changeDelay << std::endl;
            file << "HasPhysicsTied: " << profile.hasPhysicsTied << std::endl;
            file << "Notes: " << profile.notes << std::endl;
            
            file.close();
            std::wcout << L"프로필이 저장되었습니다: " << std::wstring(filename.begin(), filename.end()) << std::endl;
        }
    }
};

int main() {
    std::wcout << L"=== 안전한 FPS 변경 시스템 ===" << std::endl;
    std::wcout << L"게임별 안전 범위를 확인하여 안전하게 FPS를 변경합니다." << std::endl;
    
    SafeFPSChanger fpsChanger;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 시스템 초기화
    if (!fpsChanger.Initialize(processName)) {
        std::wcout << L"시스템 초기화 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 게임 프로필 보기" << std::endl;
        std::wcout << L"2. 권장 FPS 보기" << std::endl;
        std::wcout << L"3. 안전한 FPS 변경" << std::endl;
        std::wcout << L"4. FPS 안정성 테스트" << std::endl;
        std::wcout << L"5. 커스텀 프로필 생성" << std::endl;
        std::wcout << L"6. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1:
                fpsChanger.ShowGameProfile();
                break;
                
            case 2:
                fpsChanger.ShowRecommendedFPS();
                break;
                
            case 3: {
                std::wcout << L"FPS 주소를 입력하세요 (16진수): 0x";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                
                std::wcout << L"목표 FPS를 입력하세요: ";
                float targetFPS;
                std::wcin >> targetFPS;
                
                fpsChanger.SafeChangeFPS(address, targetFPS);
                break;
            }
            
            case 4: {
                std::wcout << L"FPS 주소를 입력하세요 (16진수): 0x";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                
                std::wcout << L"테스트할 FPS를 입력하세요: ";
                float testFPS;
                std::wcin >> testFPS;
                
                std::wcout << L"테스트 시간 (초, 기본 10): ";
                int duration;
                std::wcin >> duration;
                if (duration <= 0) duration = 10;
                
                fpsChanger.TestFPSStability(address, testFPS, duration);
                break;
            }
            
            case 5:
                fpsChanger.CreateCustomProfile();
                break;
                
            case 6:
                std::wcout << L"프로그램을 종료합니다." << std::endl;
                return 0;
                
            default:
                std::wcout << L"잘못된 선택입니다." << std::endl;
                break;
        }
    }
    
    return 0;
}