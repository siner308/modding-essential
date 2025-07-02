/*
 * Exercise 4: FPS 모니터링
 * 
 * 문제: 실시간으로 FPS를 측정하고 표시하는 모니터를 작성하세요.
 * 
 * 학습 목표:
 * - 정확한 FPS 측정 기법
 * - 실시간 데이터 시각화
 * - 성능 분석 도구 구현
 */

#include <Windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <map>
#include <cmath> // For std::isfinite, std::sqrt

class FPSMonitor {
private:
    HANDLE processHandle;
    DWORD processId;
    std::wstring processName;
    
    struct FPSReading {
        std::chrono::system_clock::time_point timestamp;
        float fps;
        float frameTime; // milliseconds
        DWORD cpuUsage;
        SIZE_T memoryUsage;
    };
    
    struct MonitorConfig {
        int updateInterval; // milliseconds
        int historySize;    // number of readings to keep
        bool showGraph;
        bool logToFile;
        bool showStatistics;
        float warningThreshold;
        float criticalThreshold;
    };
    
    struct Statistics {
        float avgFPS;
        float minFPS;
        float maxFPS;
        float avgFrameTime;
        float minFrameTime;
        float maxFrameTime;
        float variance;
        float standardDeviation;
        int dropCount;
        int spikeCount;
        float stability; // percentage
    };
    
    std::deque<FPSReading> readings;
    MonitorConfig config;
    std::vector<uintptr_t> monitorAddresses;
    bool isMonitoring;
    std::thread monitorThread;
    std::ofstream logFile;

public:
    FPSMonitor() : processHandle(nullptr), processId(0), isMonitoring(false) {
        InitializeConfig();
    }
    
    ~FPSMonitor() {
        StopMonitoring();
        if (processHandle) {
            CloseHandle(processHandle);
        }
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void InitializeConfig() {
        config.updateInterval = 100; // 0.1초마다 업데이트
        config.historySize = 600;    // 60초 분량 (0.1초 * 600)
        config.showGraph = true;
        config.logToFile = false;
        config.showStatistics = true;
        config.warningThreshold = 45.0f;  // 45 FPS 이하 경고
        config.criticalThreshold = 30.0f; // 30 FPS 이하 위험
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
        
        std::wcout << L"FPS 모니터 초기화 완료" << std::endl;
        return true;
    }
    
    void AddMonitorAddress(uintptr_t address) {
        monitorAddresses.push_back(address);
        std::wcout << L"모니터링 주소 추가: 0x" << std::hex << address << std::dec << std::endl;
    }
    
    void StartMonitoring() {
        if (isMonitoring) {
            std::wcout << L"이미 모니터링 중입니다." << std::endl;
            return;
        }
        
        if (monitorAddresses.empty()) {
            std::wcout << L"모니터링할 주소가 없습니다." << std::endl;
            return;
        }
        
        isMonitoring = true;
        readings.clear();
        
        // 로그 파일 설정
        if (config.logToFile) {
            std::string logFileName = "fps_log_" + GetCurrentTimeString() + ".csv";
            logFile.open(logFileName);
            if (logFile.is_open()) {
                logFile << "Timestamp,FPS,FrameTime,CPUUsage,MemoryUsage" << std::endl;
                std::wcout << L"로그 파일 생성: " << std::wstring(logFileName.begin(), logFileName.end()) << std::endl;
            }
        }
        
        // 모니터링 스레드 시작
        monitorThread = std::thread(&FPSMonitor::MonitoringLoop, this);
        
        std::wcout << L"FPS 모니터링 시작..." << std::endl;
        std::wcout << L"ESC 키를 눌러 중지하세요." << std::endl;
    }
    
    void StopMonitoring() {
        if (!isMonitoring) {
            return;
        }
        
        isMonitoring = false;
        
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
        
        if (logFile.is_open()) {
            logFile.close();
        }
        
        std::wcout << L"\nFPS 모니터링 중지" << std::endl;
    }
    
    void MonitoringLoop() {
        auto lastUpdate = std::chrono::system_clock::now();
        
        while (isMonitoring) {
            auto now = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
            
            if (elapsed.count() >= config.updateInterval) {
                UpdateReadings(now);
                UpdateDisplay();
                lastUpdate = now;
            }
            
            // ESC 키 확인
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        isMonitoring = false;
    }
    
    void UpdateReadings(std::chrono::system_clock::time_point timestamp) {
        // 모든 주소에서 FPS 값 읽기 (첫 번째 유효한 값 사용)
        float currentFPS = 0.0f;
        bool validReading = false;
        
        for (uintptr_t address : monitorAddresses) {
            float fps;
            SIZE_T bytesRead;
            
            if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address),
                                 &fps, sizeof(fps), &bytesRead) && bytesRead == sizeof(fps)) {
                
                if (fps > 0.0f && fps < 1000.0f && std::isfinite(fps)) {
                    currentFPS = fps;
                    validReading = true;
                    break;
                }
            }
        }
        
        if (!validReading) {
            return; // 유효한 읽기 없음
        }
        
        // FPS 읽기 생성
        FPSReading reading;
        reading.timestamp = timestamp;
        reading.fps = currentFPS;
        reading.frameTime = (currentFPS > 0) ? (1000.0f / currentFPS) : 0.0f;
        reading.cpuUsage = GetProcessCPUUsage();
        reading.memoryUsage = GetProcessMemoryUsage();
        
        // 히스토리에 추가
        readings.push_back(reading);
        
        // 히스토리 크기 제한
        while (readings.size() > static_cast<size_t>(config.historySize)) {
            readings.pop_front();
        }
        
        // 로그 파일에 기록
        if (logFile.is_open()) {
            logFile << GetTimeString(timestamp) << ","
                    << reading.fps << ","
                    << reading.frameTime << ","
                    << reading.cpuUsage << ","
                    << reading.memoryUsage << std::endl;
        }
    }
    
    void UpdateDisplay() {
        // 콘솔 화면 지우기
        // system("cls"); // Windows 전용, 깜빡임 발생 가능
        // 대신 커서 이동 및 덮어쓰기 방식 사용
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord = {0, 0};
        SetConsoleCursorPosition(hConsole, coord);
        
        std::wcout << L"=== FPS 모니터 ===" << std::endl;
        std::wcout << L"프로세스: " << processName << L" (PID: " << processId << L")" << std::endl;
        std::wcout << L"모니터링 주소 수: " << monitorAddresses.size() << std::endl;
        std::wcout << L"샘플 수: " << readings.size() << L"/" << config.historySize << std::endl;
        
        if (readings.empty()) {
            std::wcout << L"\n데이터 수집 중..." << std::endl;
            return;
        }
        
        // 현재 값 표시
        const auto& latest = readings.back();
        std::wcout << L"\n현재 FPS: " << std::fixed << std::setprecision(1) << latest.fps;
        
        // 상태 표시
        if (latest.fps < config.criticalThreshold) {
            std::wcout << L" [위험]";
        } else if (latest.fps < config.warningThreshold) {
            std::wcout << L" [경고]";
        } else {
            std::wcout << L" [정상]";
        }
        
        std::wcout << std::endl;
        std::wcout << L"프레임 시간: " << latest.frameTime << L" ms" << std::endl;
        std::wcout << L"CPU 사용률: " << latest.cpuUsage << L"%" << std::endl;
        std::wcout << L"메모리 사용량: " << (latest.memoryUsage / 1024 / 1024) << L" MB" << std::endl;
        
        // 통계 표시
        if (config.showStatistics && readings.size() >= 10) {
            ShowStatistics();
        }
        
        // 그래프 표시
        if (config.showGraph) {
            ShowGraph();
        }
        
        std::wcout << L"\nESC: 중지 | F1: 설정 | F2: 로그 토글 | F3: 그래프 토글" << std::endl;
        
        // 키 입력 처리
        HandleKeyInput();
    }
    
    void ShowStatistics() {
        Statistics stats = CalculateStatistics();
        
        std::wcout << L"\n=== 통계 (최근 " << readings.size() << L"개 샘플) ===" << std::endl;
        std::wcout << L"평균 FPS: " << std::fixed << std::setprecision(1) << stats.avgFPS << std::endl;
        std::wcout << L"최소/최대 FPS: " << stats.minFPS << L" / " << stats.maxFPS << std::endl;
        std::wcout << L"평균 프레임 시간: " << stats.avgFrameTime << L" ms" << std::endl;
        std::wcout << L"표준편차: " << stats.standardDeviation << std::endl;
        std::wcout << L"안정성: " << stats.stability << L"%" << std::endl;
        std::wcout << L"드롭/스파이크: " << stats.dropCount << L" / " << stats.spikeCount << std::endl;
    }
    
    Statistics CalculateStatistics() {
        Statistics stats = {};
        
        if (readings.empty()) {
            return stats;
        }
        
        std::vector<float> fpsValues;
        std::vector<float> frameTimeValues;
        
        for (const auto& reading : readings) {
            fpsValues.push_back(reading.fps);
            frameTimeValues.push_back(reading.frameTime);
        }
        
        // 기본 통계
        stats.minFPS = *std::min_element(fpsValues.begin(), fpsValues.end());
        stats.maxFPS = *std::max_element(fpsValues.begin(), fpsValues.end());
        stats.minFrameTime = *std::min_element(frameTimeValues.begin(), frameTimeValues.end());
        stats.maxFrameTime = *std::max_element(frameTimeValues.begin(), frameTimeValues.end());
        
        // 평균
        float sumFPS = 0, sumFrameTime = 0;
        for (size_t i = 0; i < fpsValues.size(); ++i) {
            sumFPS += fpsValues[i];
            sumFrameTime += frameTimeValues[i];
        }
        stats.avgFPS = sumFPS / fpsValues.size();
        stats.avgFrameTime = sumFrameTime / frameTimeValues.size();
        
        // 분산과 표준편차
        float varianceSum = 0;
        for (float fps : fpsValues) {
            float diff = fps - stats.avgFPS;
            varianceSum += diff * diff;
        }
        stats.variance = varianceSum / fpsValues.size();
        stats.standardDeviation = std::sqrt(stats.variance);
        
        // 드롭과 스파이크 계산
        stats.dropCount = 0;
        stats.spikeCount = 0;
        
        for (size_t i = 1; i < fpsValues.size(); ++i) {
            float change = fpsValues[i] - fpsValues[i-1];
            if (change < -10.0f) stats.dropCount++;
            if (change > 15.0f) stats.spikeCount++;
        }
        
        // 안정성 계산 (표준편차 기반)
        float maxAcceptableDeviation = stats.avgFPS * 0.1f; // 10%
        stats.stability = std::max(0.0f, 100.0f - (stats.standardDeviation / maxAcceptableDeviation * 100.0f));
        
        return stats;
    }
    
    void ShowGraph() {
        const int graphWidth = 60;
        const int graphHeight = 10;
        
        std::wcout << L"\n=== FPS 그래프 (최근 " << std::min<size_t>(readings.size(), graphWidth) << L"개) ===" << std::endl;
        
        if (readings.size() < 2) {
            std::wcout << L"그래프를 표시하기에 데이터가 부족합니다." << std::endl;
            return;
        }
        
        // 그래프 데이터 준비
        size_t startIdx = readings.size() > graphWidth ? readings.size() - graphWidth : 0;
        std::vector<float> graphData;
        
        for (size_t i = startIdx; i < readings.size(); ++i) {
            graphData.push_back(readings[i].fps);
        }
        
        // 그래프 범위 계산
        float minVal = *std::min_element(graphData.begin(), graphData.end());
        float maxVal = *std::max_element(graphData.begin(), graphData.end());
        
        // 최소 범위 보장
        if (maxVal - minVal < 10.0f) {
            float center = (minVal + maxVal) / 2;
            minVal = center - 5.0f;
            maxVal = center + 5.0f;
        }
        
        // 그래프 그리기
        for (int row = graphHeight - 1; row >= 0; --row) {
            float threshold = minVal + (maxVal - minVal) * row / (graphHeight - 1);
            
            std::wcout << std::fixed << std::setprecision(0) << std::setw(3) << threshold << L" |";
            
            for (size_t col = 0; col < graphData.size(); ++col) {
                if (graphData[col] >= threshold) {
                    if (graphData[col] < config.criticalThreshold) {
                        std::wcout << L"#"; // 위험 (빨간색 대신)
                    } else if (graphData[col] < config.warningThreshold) {
                        std::wcout << L"*"; // 경고 (노란색 대신)
                    } else {
                        std::wcout << L"▆"; // 정상 (녹색 대신)
                    }
                } else {
                    std::wcout << L" ";
                }
            }
            std::wcout << std::endl;
        }
        
        // X축
        std::wcout << L"    +";
        for (size_t i = 0; i < graphData.size(); ++i) {
            std::wcout << L"-";
        }
        std::wcout << L">" << std::endl;
        
        std::wcout << L"    범위: " << minVal << L" - " << maxVal << L" FPS" << std::endl;
        std::wcout << L"    범례: ▆=정상, *=경고, #=위험" << std::endl;
    }
    
    void HandleKeyInput() {
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            ShowConfigMenu();
            Sleep(200); // 키 반복 방지
        }
        
        if (GetAsyncKeyState(VK_F2) & 0x8000) {
            config.logToFile = !config.logToFile;
            Sleep(200);
        }
        
        if (GetAsyncKeyState(VK_F3) & 0x8000) {
            config.showGraph = !config.showGraph;
            Sleep(200);
        }
    }
    
    void ShowConfigMenu() {
        std::wcout << L"\n=== 설정 메뉴 ===" << std::endl;
        std::wcout << L"1. 업데이트 간격: " << config.updateInterval << L" ms" << std::endl;
        std::wcout << L"2. 히스토리 크기: " << config.historySize << std::endl;
        std::wcout << L"3. 경고 임계값: " << config.warningThreshold << L" FPS" << std::endl;
        std::wcout << L"4. 위험 임계값: " << config.criticalThreshold << L" FPS" << std::endl;
        std::wcout << L"5. 그래프 표시: " << (config.showGraph ? L"켜짐" : L"꺼짐") << std::endl;
        std::wcout << L"6. 로그 파일: " << (config.logToFile ? L"켜짐" : L"꺼짐") << std::endl;
        std::wcout << L"\n변경할 항목 (1-6, 0=취소): ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1:
                std::wcout << L"새 업데이트 간격 (ms): ";
                std::wcin >> config.updateInterval;
                break;
            case 2:
                std::wcout << L"새 히스토리 크기: ";
                std::wcin >> config.historySize;
                break;
            case 3:
                std::wcout << L"새 경고 임계값: ";
                std::wcin >> config.warningThreshold;
                break;
            case 4:
                std::wcout << L"새 위험 임계값: ";
                std::wcin >> config.criticalThreshold;
                break;
            case 5:
                config.showGraph = !config.showGraph;
                break;
            case 6:
                config.logToFile = !config.logToFile;
                break;
        }
    }
    
    void ExportStatistics() {
        if (readings.empty()) {
            std::wcout << L"내보낼 데이터가 없습니다." << std::endl;
            return;
        }
        
        std::string filename = "fps_stats_" + GetCurrentTimeString() + ".txt";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::wcout << L"파일 생성 실패" << std::endl;
            return;
        }
        
        Statistics stats = CalculateStatistics();
        
        file << "FPS Monitoring Statistics Report" << std::endl;
        file << "=================================" << std::endl;
        file << "Process: " << std::string(processName.begin(), processName.end()) << std::endl;
        file << "Monitor Duration: " << readings.size() * config.updateInterval / 1000.0 << " seconds" << std::endl;
        file << "Sample Count: " << readings.size() << std::endl;
        file << std::endl;
        
        file << "FPS Statistics:" << std::endl;
        file << "Average FPS: " << stats.avgFPS << std::endl;
        file << "Minimum FPS: " << stats.minFPS << std::endl;
        file << "Maximum FPS: " << stats.maxFPS << std::endl;
        file << "Standard Deviation: " << stats.standardDeviation << std::endl;
        file << "Stability: " << stats.stability << "%" << std::endl;
        file << std::endl;
        
        file << "Frame Time Statistics:" << std::endl;
        file << "Average Frame Time: " << stats.avgFrameTime << " ms" << std::endl;
        file << "Minimum Frame Time: " << stats.minFrameTime << " ms" << std::endl;
        file << "Maximum Frame Time: " << stats.maxFrameTime << " ms" << std::endl;
        file << std::endl;
        
        file << "Performance Events:" << std::endl;
        file << "FPS Drops: " << stats.dropCount << std::endl;
        file << "FPS Spikes: " << stats.spikeCount << std::endl;
        
        file.close();
        
        std::wcout << L"통계 리포트가 저장되었습니다: " << std::wstring(filename.begin(), filename.end()) << std::endl;
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
            } while (Process32NextW(snapshot, &processEntry));
        }
        
        CloseHandle(snapshot);
        return found;
    }
    
    DWORD GetProcessCPUUsage() {
        static ULARGE_INTEGER lastCPU = {};
        static ULARGE_INTEGER lastSysCPU = {};
        static ULARGE_INTEGER lastUserCPU = {};
        static DWORD numProcessors = 0;
        
        if (numProcessors == 0) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            numProcessors = sysInfo.dwNumberOfProcessors;
        }
        
        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;
        
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&now, &ftime, sizeof(FILETIME));
        
        GetProcessTimes(processHandle, &ftime, &ftime, &fsys, &fuser);
        memcpy(&sys, &fsys, sizeof(FILETIME));
        memcpy(&user, &fuser, sizeof(FILETIME));
        
        if (lastCPU.QuadPart != 0) {
            double percent = static_cast<double>((sys.QuadPart - lastSysCPU.QuadPart) + 
                                               (user.QuadPart - lastUserCPU.QuadPart));
            percent /= (now.QuadPart - lastCPU.QuadPart);
            percent /= numProcessors;
            percent *= 100;
            
            lastCPU = now;
            lastUserCPU = user;
            lastSysCPU = sys;
            
            return static_cast<DWORD>(percent);
        }
        
        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;
        
        return 0;
    }
    
    SIZE_T GetProcessMemoryUsage() {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
    }
    
    std::string GetCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        return ss.str();
    }
    
    std::string GetTimeString(std::chrono::system_clock::time_point timePoint) {
        auto time_t = std::chrono::system_clock::to_time_t(timePoint);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timePoint.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
};

int main() {
    std::wcout << L"=== FPS 실시간 모니터 ===" << std::endl;
    std::wcout << L"게임의 FPS를 실시간으로 측정하고 분석합니다." << std::endl;
    
    FPSMonitor monitor;
    
    // 프로세스 이름 입력
    std::wcout << L"\n대상 게임 프로세스 이름을 입력하세요 (예: eldenring.exe): ";
    std::wstring processName;
    std::wcin >> processName;
    
    // 시스템 초기화
    if (!monitor.Initialize(processName)) {
        std::wcout << L"시스템 초기화 실패" << std::endl;
        std::wcin.ignore();
        std::wcin.get();
        return 1;
    }
    
    // 메인 루프
    while (true) {
        std::wcout << L"\n=== 메뉴 ===" << std::endl;
        std::wcout << L"1. 모니터링 주소 추가" << std::endl;
        std::wcout << L"2. 모니터링 시작" << std::endl;
        std::wcout << L"3. 모니터링 중지" << std::endl;
        std::wcout << L"4. 통계 내보내기" << std::endl;
        std::wcout << L"5. 종료" << std::endl;
        std::wcout << L"선택: ";
        
        int choice;
        std::wcin >> choice;
        
        switch (choice) {
            case 1: {
                std::wcout << L"모니터링할 FPS 주소를 입력하세요 (16진수): 0x";
                uintptr_t address;
                std::wcin >> std::hex >> address >> std::dec;
                monitor.AddMonitorAddress(address);
                break;
            }
            
            case 2:
                monitor.StartMonitoring();
                break;
                
            case 3:
                monitor.StopMonitoring();
                break;
                
            case 4:
                monitor.ExportStatistics();
                break;
                
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