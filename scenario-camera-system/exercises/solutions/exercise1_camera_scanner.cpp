/*
 * Exercise 1: 카메라 주소 찾기
 * 
 * 문제: 게임의 카메라 위치와 회전 정보가 저장된 메모리 주소를 찾는 스캐너를 작성하세요.
 * 
 * 학습 목표:
 * - 카메라 메모리 구조 이해
 * - 메모리 패턴 매칭
 * - 동적 주소 탐지
 */

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <DirectXMath.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath> // For std::abs, std::isfinite, std::sqrt

#pragma comment(lib, "psapi.lib")

using namespace DirectX;

struct CameraData {
    XMFLOAT3 position;      // 0x00: 카메라 위치
    XMFLOAT3 rotation;      // 0x0C: 오일러 각도 (pitch, yaw, roll)
    float fov;              // 0x18: 시야각 (radians)
    float nearPlane;        // 0x1C: 근거리 클리핑
    float farPlane;         // 0x20: 원거리 클리핑
    float aspectRatio;      // 0x24: 화면 비율
    char padding[8];        // 0x28: 패딩
};

class CameraScanner {
private:
    HANDLE processHandle;
    std::vector<MEMORY_BASIC_INFORMATION> memoryRegions;
    std::vector<uintptr_t> candidateAddresses;
    
    // 스캔 결과 저장
    struct ScanResult {
        uintptr_t address;
        CameraData data;
        float confidence;
        std::string description;
    };
    
    std::vector<ScanResult> scanResults;
    
    // 스캔 패턴들
    struct ScanPattern {
        std::string name;
        std::vector<uint8_t> pattern;
        std::vector<bool> mask;
        int offset;
    };
    
    std::vector<ScanPattern> patterns;
    
    // 게임 엔진별 패턴
    void InitializePatterns() {
        // Unreal Engine 카메라 패턴
        patterns.push_back({
            "UE4_Camera_Position",
            {0xF3, 0x0F, 0x11, 0x40, 0x00, 0xF3, 0x0F, 0x11, 0x48, 0x00, 0xF3, 0x0F, 0x11, 0x50, 0x00},
            {true, true, true, true, false, true, true, true, true, false, true, true, true, true, false},
            -16  // 패턴 이전 16바이트가 실제 데이터
        });
        
        // Unity 카메라 패턴
        patterns.push_back({
            "Unity_Camera_Transform",
            {0x48, 0x8B, 0x80, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x74, 0x00, 0x48, 0x8B, 0x40},
            {true, true, true, false, false, false, false, true, true, true, true, false, true, true, true},
            0
        });
        
        // FromSoftware 카메라 패턴 (Elden Ring, Dark Souls)
        patterns.push_back({
            "FromSoft_Camera_Data",
            {0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC9, 0x74, 0x00, 0x0F, 0x28, 0x05},
            {true, true, true, false, false, false, false, true, true, true, true, false, true, true, true},
            0
        });
        
        // 일반적인 FOV 패턴
        patterns.push_back({
            "Generic_FOV_Pattern",
            {0x89, 0x81, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x81, 0x00, 0x00, 0x00, 0x00},
            {true, true, false, false, false, false, true, true, false, false, false, false},
            -24  // FOV 이전 24바이트가 position
        });
    }

public:
    CameraScanner() : processHandle(nullptr) {
        InitializePatterns();
    }
    
    ~CameraScanner() {
        if (processHandle && processHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(processHandle);
        }
    }
    
    bool Initialize(const std::wstring& processName) {
        std::wcout << L"카메라 스캐너 초기화 중..." << std::endl;
        
        // 프로세스 찾기
        DWORD processId = FindProcessByName(processName);
        if (processId == 0) {
            std::wcout << L"프로세스를 찾을 수 없습니다: " << processName << std::endl;
            return false;
        }
        
        // 프로세스 핸들 획득
        processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
        if (!processHandle) {
            std::wcout << L"프로세스 핸들을 열 수 없습니다." << std::endl;
            return false;
        }
        
        std::wcout << L"프로세스 연결 성공: " << processName << L" (PID: " << processId << L")" << std::endl;
        
        // 메모리 영역 탐색
        ScanMemoryRegions();
        
        return true;
    }
    
    void StartFullScan() {
        std::wcout << L"전체 카메라 스캔 시작..." << std::endl;
        
        // 여러 방법으로 스캔
        ScanByValueRange();
        ScanByPatternMatching();
        ScanByStructureAnalysis();
        ScanByRuntimeAnalysis();
        
        // 결과 분석 및 출력
        AnalyzeResults();
        PrintResults();
    }
    
private:
    DWORD FindProcessByName(const std::wstring& processName) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }
        
        PROCESSENTRY32W pe32 = {};
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (processName == pe32.szExeFile) {
                    CloseHandle(hSnapshot);
                    return pe32.th32ProcessID;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return 0;
    }
    
    void ScanMemoryRegions() {
        std::wcout << L"메모리 영역 스캔 중..." << std::endl;
        
        memoryRegions.clear();
        
        uintptr_t address = 0;
        MEMORY_BASIC_INFORMATION mbi;
        
        while (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_READONLY)) {
                memoryRegions.push_back(mbi);
            }
            address += mbi.RegionSize;
        }
        
        std::wcout << L"스캔 가능한 메모리 영역: " << memoryRegions.size() << L"개" << std::endl;
    }
    
    void ScanByValueRange() {
        std::wcout << L"값 범위 기반 스캔 중..." << std::endl;
        
        // 일반적인 FOV 값들 (60도 ~ 120도)
        std::vector<float> fovValues = {
            XMConvertToRadians(60.0f),
            XMConvertToRadians(70.0f),
            XMConvertToRadians(75.0f),
            XMConvertToRadians(80.0f),
            XMConvertToRadians(85.0f),
            XMConvertToRadians(90.0f),
            XMConvertToRadians(95.0f),
            XMConvertToRadians(100.0f),
            XMConvertToRadians(105.0f),
            XMConvertToRadians(110.0f),
            XMConvertToRadians(120.0f)
        };
        
        for (const auto& region : memoryRegions) {
            std::vector<uint8_t> buffer(region.RegionSize);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, region.BaseAddress, buffer.data(), region.RegionSize, &bytesRead)) {
                for (size_t i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
                    float value = *reinterpret_cast<float*>(&buffer[i]);
                    
                    for (float fov : fovValues) {
                        if (std::abs(value - fov) < 0.01f) {
                            uintptr_t address = reinterpret_cast<uintptr_t>(region.BaseAddress) + i;
                            ValidateCameraStructure(address);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    void ScanByPatternMatching() {
        std::wcout << L"패턴 매칭 스캔 중..." << std::endl;
        
        for (const auto& pattern : patterns) {
            std::wcout << L"패턴 스캔: " << std::wstring(pattern.name.begin(), pattern.name.end()) << std::endl;
            
            for (const auto& region : memoryRegions) {
                std::vector<uint8_t> buffer(region.RegionSize);
                SIZE_T bytesRead;
                
                if (ReadProcessMemory(processHandle, region.BaseAddress, buffer.data(), region.RegionSize, &bytesRead)) {
                    std::vector<uintptr_t> matches = FindPattern(buffer, pattern.pattern, pattern.mask);
                    
                    for (uintptr_t offset : matches) {
                        uintptr_t address = reinterpret_cast<uintptr_t>(region.BaseAddress) + offset + pattern.offset;
                        ValidateCameraStructure(address);
                    }
                }
            }
        }
    }
    
    void ScanByStructureAnalysis() {
        std::wcout << L"구조체 분석 스캔 중..." << std::endl;
        
        for (const auto& region : memoryRegions) {
            std::vector<uint8_t> buffer(region.RegionSize);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, region.BaseAddress, buffer.data(), region.RegionSize, &bytesRead)) {
                // 가능한 카메라 구조체 위치 찾기
                for (size_t i = 0; i <= bytesRead - sizeof(CameraData); i += 16) {
                    CameraData* data = reinterpret_cast<CameraData*>(&buffer[i]);
                    
                    if (IsPotentialCameraData(*data)) {
                        uintptr_t address = reinterpret_cast<uintptr_t>(region.BaseAddress) + i;
                        ValidateCameraStructure(address);
                    }
                }
            }
        }
    }
    
    void ScanByRuntimeAnalysis() {
        std::wcout << L"런타임 분석 스캔 중..." << std::endl;
        
        // 런타임에 값이 변경되는 주소 찾기
        std::vector<uintptr_t> initialAddresses;
        
        // 첫 번째 스냅샷
        std::map<uintptr_t, float> initialValues;
        for (const auto& region : memoryRegions) {
            std::vector<uint8_t> buffer(region.RegionSize);
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, region.BaseAddress, buffer.data(), region.RegionSize, &bytesRead)) {
                for (size_t i = 0; i <= bytesRead - sizeof(float); i += sizeof(float)) {
                    float value = *reinterpret_cast<float*>(&buffer[i]);
                    if (IsReasonableValue(value)) {
                        uintptr_t address = reinterpret_cast<uintptr_t>(region.BaseAddress) + i;
                        initialValues[address] = value;
                    }
                }
            }
        }
        
        std::wcout << L"초기 값 수집 완료. 5초 대기 후 변경 사항 확인..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 두 번째 스냅샷으로 변경된 값 찾기
        for (const auto& pair : initialValues) {
            uintptr_t address = pair.first;
            float initialValue = pair.second;
            
            float currentValue;
            SIZE_T bytesRead;
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &currentValue, sizeof(float), &bytesRead)) {
                if (std::abs(currentValue - initialValue) > 0.01f) {
                    // 값이 변경된 주소 - 카메라 데이터일 가능성
                    ValidateCameraStructure(address - 24); // position 시작점 추정
                }
            }
        }
    }
    
    bool IsPotentialCameraData(const CameraData& data) {
        // 위치 값 검증
        if (std::abs(data.position.x) > 100000.0f || 
            std::abs(data.position.y) > 100000.0f || 
            std::abs(data.position.z) > 100000.0f) {
            return false;
        }
        
        // 회전 값 검증 (라디안)
        if (std::abs(data.rotation.x) > XM_2PI || 
            std::abs(data.rotation.y) > XM_2PI || 
            std::abs(data.rotation.z) > XM_2PI) {
            return false;
        }
        
        // FOV 검증
        if (data.fov < XMConvertToRadians(10.0f) || 
            data.fov > XMConvertToRadians(180.0f)) {
            return false;
        }
        
        // 클리핑 평면 검증
        if (data.nearPlane <= 0.0f || data.farPlane <= data.nearPlane) {
            return false;
        }
        
        // 화면 비율 검증
        if (data.aspectRatio < 0.5f || data.aspectRatio > 3.0f) {
            return false;
        }
        
        return true;
    }
    
    bool IsReasonableValue(float value) {
        if (!std::isfinite(value)) return false;
        if (std::abs(value) > 1000000.0f) return false;
        return true;
    }
    
    void ValidateCameraStructure(uintptr_t address) {
        CameraData data;
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &data, sizeof(CameraData), &bytesRead)) {
            return;
        }
        
        if (!IsPotentialCameraData(data)) {
            return;
        }
        
        // 중복 주소 확인
        for (const auto& result : scanResults) {
            if (result.address == address) {
                return;
            }
        }
        
        // 신뢰도 계산
        float confidence = CalculateConfidence(data);
        
        ScanResult result;
        result.address = address;
        result.data = data;
        result.confidence = confidence;
        result.description = GenerateDescription(data);
        
        scanResults.push_back(result);
        
        std::wcout << L"카메라 후보 발견: 0x" << std::hex << address 
                  << L" (신뢰도: " << std::fixed << std::setprecision(2) << confidence << L")" << std::endl;
    }
    
    float CalculateConfidence(const CameraData& data) {
        float confidence = 0.0f;
        
        // 위치 합리성 (거리 기준)
        float distance = std::sqrt(data.position.x * data.position.x + 
                             data.position.y * data.position.y + 
                             data.position.z * data.position.z);
        if (distance > 0.1f && distance < 10000.0f) {
            confidence += 0.3f;
        }
        
        // FOV 합리성
        float fovDegrees = XMConvertToDegrees(data.fov);
        if (fovDegrees >= 60.0f && fovDegrees <= 120.0f) {
            confidence += 0.3f;
        }
        
        // 클리핑 평면 합리성
        if (data.nearPlane > 0.01f && data.nearPlane < 10.0f && 
            data.farPlane > 100.0f && data.farPlane < 100000.0f) {
            confidence += 0.2f;
        }
        
        // 화면 비율 합리성
        if (data.aspectRatio >= 1.0f && data.aspectRatio <= 2.5f) {
            confidence += 0.2f;
        }
        
        return confidence;
    }
    
    std::string GenerateDescription(const CameraData& data) {
        std::stringstream ss;
        
        ss << "Position: (" << std::fixed << std::setprecision(2) 
           << data.position.x << ", " << data.position.y << ", " << data.position.z << ") ";
        
        ss << "Rotation: (" << std::fixed << std::setprecision(1)
           << XMConvertToDegrees(data.rotation.x) << "°, " 
           << XMConvertToDegrees(data.rotation.y) << "°, " 
           << XMConvertToDegrees(data.rotation.z) << "°) ";
        
        ss << "FOV: " << std::fixed << std::setprecision(1) 
           << XMConvertToDegrees(data.fov) << "° ";
        
        ss << "Aspect: " << std::fixed << std::setprecision(2) 
           << data.aspectRatio;
        
        return ss.str();
    }
    
    std::vector<uintptr_t> FindPattern(const std::vector<uint8_t>& buffer, 
                                      const std::vector<uint8_t>& pattern, 
                                      const std::vector<bool>& mask) {
        std::vector<uintptr_t> matches;
        
        for (size_t i = 0; i <= buffer.size() - pattern.size(); i++) {
            bool match = true;
            for (size_t j = 0; j < pattern.size(); j++) {
                if (mask[j] && buffer[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                matches.push_back(i);
            }
        }
        
        return matches;
    }
    
    void AnalyzeResults() {
        std::wcout << L"결과 분석 중..." << std::endl;
        
        // 신뢰도 순으로 정렬
        std::sort(scanResults.begin(), scanResults.end(), 
                  [](const ScanResult& a, const ScanResult& b) {
                      return a.confidence > b.confidence;
                  });
        
        // 중복 근사 주소 제거
        std::vector<ScanResult> filteredResults;
        for (const auto& result : scanResults) {
            bool isDuplicate = false;
            for (const auto& existing : filteredResults) {
                if (std::abs(static_cast<long long>(result.address) - static_cast<long long>(existing.address)) < 256) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                filteredResults.push_back(result);
            }
        }
        
        scanResults = filteredResults;
    }
    
    void PrintResults() {
        std::wcout << L"\n=== 카메라 스캔 결과 ===" << std::endl;
        
        if (scanResults.empty()) {
            std::wcout << L"카메라 데이터를 찾을 수 없습니다." << std::endl;
            return;
        }
        
        std::wcout << L"총 " << scanResults.size() << L"개의 카메라 후보 발견" << std::endl;
        std::wcout << L"신뢰도 순 정렬:" << std::endl;
        
        for (size_t i = 0; i < std::min(scanResults.size(), size_t(10)); i++) {
            const auto& result = scanResults[i];
            
            std::wcout << L"\n[" << (i + 1) << L"] 주소: 0x" << std::hex << result.address 
                      << L" (신뢰도: " << std::fixed << std::setprecision(2) << result.confidence << L")" << std::endl;
            
            std::wcout << L"    " << StringToWString(result.description) << std::endl;
        }
        
        // 최고 신뢰도 주소 추천
        if (scanResults[0].confidence > 0.7f) {
            std::wcout << L"\n권장 카메라 주소: 0x" << std::hex << scanResults[0].address << std::endl;
        }
        
        // 결과를 파일로 저장
        SaveResultsToFile();
    }
    
    void SaveResultsToFile() {
        std::ofstream file("camera_scan_results.txt");
        if (!file.is_open()) return;
        
        file << "Camera Scan Results\n";
        file << "==================\n\n";
        
        for (size_t i = 0; i < scanResults.size(); i++) {
            const auto& result = scanResults[i];
            
            file << "[" << (i + 1) << "] Address: 0x" << std::hex << result.address 
                 << " (Confidence: " << std::fixed << std::setprecision(2) << result.confidence << ")\n";
            file << "    " << result.description << "\n\n";
        }
        
        file.close();
        std::wcout << L"결과가 camera_scan_results.txt에 저장되었습니다." << std::endl;
    }
};

int main() {
    std::wcout << L"=== 카메라 메모리 스캐너 ===" << std::endl;
    std::wcout << L"게임 프로세스 이름을 입력하세요 (예: EldenRing.exe): ";
    
    std::wstring processName;
    std::wcin >> processName;
    
    CameraScanner scanner;
    
    if (!scanner.Initialize(processName)) {
        std::wcout << L"초기화 실패" << std::endl;
        std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
        std::wcin.get();
        return 1;
    }
    
    std::wcout << L"스캔을 시작하려면 Enter를 누르세요..." << std::endl;
    std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
    std::wcin.get();
    
    scanner.StartFullScan();
    
    std::wcout << L"스캔 완료. 아무 키나 누르면 종료됩니다." << std::endl;
    std::wcin.get();
    
    return 0;
}