/*
 * Exercise 5: 핫 리로드 시스템
 * 
 * 문제: 개발 중 모드 파일이 변경되면 자동으로 재로드하는 기능을 만드세요.
 * 
 * 학습 목표:
 * - 파일 시스템 감시
 * - 실시간 리로딩
 * - 개발 효율성 향상
 */

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <algorithm>
#include <codecvt>
#include <locale>

namespace fs = std::filesystem;

// Helper function to convert std::string to std::wstring
std::wstring StringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Helper function to convert std::wstring to std::string
std::string WStringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

// 파일 변경 이벤트 타입
enum class FileChangeType {
    Modified,
    Created,
    Deleted,
    Renamed
};

// 파일 변경 이벤트
struct FileChangeEvent {
    std::string filename;
    std::string fullPath;
    FileChangeType changeType;
    std::chrono::system_clock::time_point timestamp;
    
    FileChangeEvent(const std::string& name, const std::string& path, FileChangeType type)
        : filename(name), fullPath(path), changeType(type), 
          timestamp(std::chrono::system_clock::now()) {}
};

// 핫 리로드 상태
enum class HotReloadState {
    Stopped,
    Running,
    Paused,
    Error
};

// 리로드 정책
struct ReloadPolicy {
    bool autoReload = true;                     // 자동 리로드 여부
    int debounceDelayMs = 1000;                 // 디바운스 지연 (ms)
    int maxRetries = 3;                         // 최대 재시도 횟수
    bool reloadDependents = true;               // 종속 모드도 리로드
    bool backupBeforeReload = true;             // 리로드 전 백업
    std::vector<std::string> excludePatterns;   // 제외할 파일 패턴
    
    ReloadPolicy() {
        // 기본 제외 패턴들
        excludePatterns.push_back("*.tmp");
        excludePatterns.push_back("*.bak");
        excludePatterns.push_back("*~");
        excludePatterns.push_back("*.swp");
    }
};

// 리로드 통계
struct ReloadStatistics {
    int totalReloads = 0;
    int successfulReloads = 0;
    int failedReloads = 0;
    int retriedReloads = 0;
    std::chrono::milliseconds totalReloadTime{0};
    std::chrono::system_clock::time_point lastReloadTime;
    std::vector<std::string> recentErrors;
    
    void AddError(const std::string& error) {
        recentErrors.push_back(error);
        if (recentErrors.size() > 10) {
            recentErrors.erase(recentErrors.begin());
        }
    }
};

// 모드 백업 정보
struct ModBackup {
    std::string originalPath;
    std::string backupPath;
    std::chrono::system_clock::time_point backupTime;
    size_t fileSize;
    
    ModBackup(const std::string& original, const std::string& backup)
        : originalPath(original), backupPath(backup), 
          backupTime(std::chrono::system_clock::now()) {
        
        std::error_code ec;
        fileSize = fs::file_size(original, ec);
        if (ec) fileSize = 0;
    }
};

// 리로드 대기 정보
struct PendingReload {
    std::string filename;
    std::string fullPath;
    std::chrono::system_clock::time_point scheduleTime;
    int retryCount = 0;
    
    PendingReload(const std::string& name, const std::string& path)
        : filename(name), fullPath(path), 
          scheduleTime(std::chrono::system_clock::now()) {}
};

class HotReloadSystem {
private:
    // 파일 감시 핸들
    HANDLE hDirectory;
    std::string watchDirectory;
    
    // 스레드 관리
    std::thread watchThread;
    std::thread reloadThread;
    std::atomic<HotReloadState> state{HotReloadState::Stopped};
    
    // 이벤트 큐
    std::queue<FileChangeEvent> eventQueue;
    std::mutex eventQueueMutex;
    std::condition_variable eventQueueCondition;
    
    // 대기 중인 리로드
    std::map<std::string, PendingReload> pendingReloads;
    std::mutex pendingReloadsMutex;
    
    // 백업 관리
    std::map<std::string, ModBackup> backups;
    std::string backupDirectory;
    
    // 설정 및 통계
    ReloadPolicy policy;
    ReloadStatistics stats;
    
    // 콜백 함수들
    std::function<bool(const std::string&)> reloadCallback;
    std::function<void(const std::string&, bool)> statusCallback;
    std::function<void(const std::string&)> logCallback;
    
    // 파일 필터링
    std::unordered_set<std::string> watchedExtensions;
    std::unordered_set<std::string> ignoredFiles;
    
    // 마지막 파일 수정 시간 추적 (디바운싱용)
    std::map<std::string, std::chrono::system_clock::time_point> lastModificationTimes;
    mutable std::mutex modificationTimesMutex;

public:
    HotReloadSystem() : hDirectory(INVALID_HANDLE_VALUE), backupDirectory("./backups") {
        // 기본 감시 확장자
        watchedExtensions.insert(".dll");
        watchedExtensions.insert(".exe");
        
        // 기본 로그 콜백
        logCallback = [](const std::string& msg) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::wcout << StringToWString("[" + std::string(std::put_time(std::localtime(&time_t), "%H:%M:%S")) 
                     + "] [HOT_RELOAD] " + msg) << std::endl;
        };
        
        CreateBackupDirectory();
    }
    
    ~HotReloadSystem() {
        Stop();
    }
    
    bool Initialize(const std::string& directory) {
        if (state != HotReloadState::Stopped) {
            LogError("Hot reload system already running");
            return false;
        }
        
        watchDirectory = directory;
        
        // 디렉토리 유효성 검사
        if (!fs::exists(watchDirectory) || !fs::is_directory(watchDirectory)) {
            LogError("Invalid watch directory: " + watchDirectory);
            return false;
        }
        
        // 디렉토리 감시 핸들 생성
        hDirectory = CreateFileA(
            watchDirectory.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );
        
        if (hDirectory == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            LogError("Failed to open directory for watching (Error " + std::to_string(error) + "): " + watchDirectory);
            return false;
        }
        
        Log("Hot reload system initialized for directory: " + watchDirectory);
        return true;
    }
    
    bool Start() {
        if (hDirectory == INVALID_HANDLE_VALUE) {
            LogError("Hot reload system not initialized");
            return false;
        }
        
        if (state != HotReloadState::Stopped) {
            LogError("Hot reload system already running");
            return false;
        }
        
        state = HotReloadState::Running;
        
        // 파일 감시 스레드 시작
        watchThread = std::thread(&HotReloadSystem::WatchThreadFunction, this);
        
        // 리로드 처리 스레드 시작
        reloadThread = std::thread(&HotReloadSystem::ReloadThreadFunction, this);
        
        Log("Hot reload system started");
        return true;
    }
    
    void Stop() {
        if (state == HotReloadState::Stopped) {
            return;
        }
        
        Log("Stopping hot reload system...");
        state = HotReloadState::Stopped;
        
        // 스레드 종료 대기
        eventQueueCondition.notify_all();
        
        if (watchThread.joinable()) {
            watchThread.join();
        }
        
        if (reloadThread.joinable()) {
            reloadThread.join();
        }
        
        // 핸들 정리
        if (hDirectory != INVALID_HANDLE_VALUE) {
            CloseHandle(hDirectory);
            hDirectory = INVALID_HANDLE_VALUE;
        }
        
        Log("Hot reload system stopped");
    }
    
    void Pause() {
        if (state == HotReloadState::Running) {
            state = HotReloadState::Paused;
            Log("Hot reload system paused");
        }
    }
    
    void Resume() {
        if (state == HotReloadState::Paused) {
            state = HotReloadState::Running;
            eventQueueCondition.notify_all();
            Log("Hot reload system resumed");
        }
    }
    
    HotReloadState GetState() const {
        return state;
    }
    
    // 콜백 함수 설정
    void SetReloadCallback(std::function<bool(const std::string&)> callback) {
        reloadCallback = callback;
    }
    
    void SetStatusCallback(std::function<void(const std::string&, bool)> callback) {
        statusCallback = callback;
    }
    
    void SetLogCallback(std::function<void(const std::string&)> callback) {
        logCallback = callback;
    }
    
    // 정책 설정
    void SetReloadPolicy(const ReloadPolicy& newPolicy) {
        policy = newPolicy;
        Log("Reload policy updated");
    }
    
    const ReloadPolicy& GetReloadPolicy() const {
        return policy;
    }
    
    // 감시할 확장자 관리
    void AddWatchedExtension(const std::string& extension) {
        std::string ext = extension;
        if (ext[0] != '.') ext = "." + ext;
        watchedExtensions.insert(ext);
        Log("Added watched extension: " + ext);
    }
    
    void RemoveWatchedExtension(const std::string& extension) {
        std::string ext = extension;
        if (ext[0] != '.') ext = "." + ext;
        watchedExtensions.erase(ext);
        Log("Removed watched extension: " + ext);
    }
    
    // 무시할 파일 관리
    void AddIgnoredFile(const std::string& filename) {
        ignoredFiles.insert(filename);
        Log("Added ignored file: " + filename);
    }
    
    void RemoveIgnoredFile(const std::string& filename) {
        ignoredFiles.erase(filename);
        Log("Removed ignored file: " + filename);
    }
    
    // 수동 리로드 트리거
    bool TriggerReload(const std::string& filename) {
        std::string fullPath = (fs::path(watchDirectory) / filename).string();
        
        if (!fs::exists(fullPath)) {
            LogError("File not found for manual reload: " + fullPath);
            return false;
        }
        
        Log("Manual reload triggered for: " + filename);
        
        {
            std::lock_guard<std::mutex> lock(eventQueueMutex);
            eventQueue.emplace(filename, fullPath, FileChangeType::Modified);
        }
        eventQueueCondition.notify_one();
        
        return true;
    }
    
    // 백업 관리
    bool CreateBackup(const std::string& filename) {
        std::string fullPath = (fs::path(watchDirectory) / filename).string();
        
        if (!fs::exists(fullPath)) {
            LogError("Cannot backup non-existent file: " + fullPath);
            return false;
        }
        
        // 백업 파일명 생성 (타임스탬프 포함)
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream backupName;
        backupName << fs::path(filename).stem().string() 
                  << "_backup_" 
                  << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
                  << fs::path(filename).extension().string();
        
        std::string backupPath = (fs::path(backupDirectory) / backupName.str()).string();
        
        // 파일 복사
        std::error_code ec;
        if (!fs::copy_file(fullPath, backupPath, ec)) {
            LogError("Failed to create backup: " + ec.message());
            return false;
        }
        
        // 백업 정보 저장
        backups[filename] = ModBackup(fullPath, backupPath);
        
        Log("Created backup: " + backupName.str());
        return true;
    }
    
    bool RestoreBackup(const std::string& filename) {
        auto it = backups.find(filename);
        if (it == backups.end()) {
            LogError("No backup found for: " + filename);
            return false;
        }
        
        const auto& backup = it->second;
        
        if (!fs::exists(backup.backupPath)) {
            LogError("Backup file not found: " + backup.backupPath);
            backups.erase(it);
            return false;
        }
        
        // 백업에서 복원
        std::error_code ec;
        if (!fs::copy_file(backup.backupPath, backup.originalPath, 
                          fs::copy_options::overwrite_existing, ec)) {
            LogError("Failed to restore backup: " + ec.message());
            return false;
        }
        
        Log("Restored backup for: " + filename);
        return true;
    }
    
    void CleanupOldBackups(int maxAge = 7) {
        auto cutoffTime = std::chrono::system_clock::now() - std::chrono::hours(24 * maxAge);
        
        std::vector<std::string> toRemove;
        
        for (const auto& [filename, backup] : backups) {
            if (backup.backupTime < cutoffTime) {
                // 백업 파일 삭제
                std::error_code ec;
                fs::remove(backup.backupPath, ec);
                if (!ec) {
                    toRemove.push_back(filename);
                }
            }
        }
        
        for (const auto& filename : toRemove) {
            backups.erase(filename);
        }
        
        if (!toRemove.empty()) {
            Log("Cleaned up " + std::to_string(toRemove.size()) + " old backups");
        }
    }
    
    // 통계 정보
    const ReloadStatistics& GetStatistics() const {
        return stats;
    }
    
    void ResetStatistics() {
        stats = ReloadStatistics();
        Log("Statistics reset");
    }
    
    void PrintStatistics() const {
        std::wcout << L"\n=== Hot Reload Statistics ===" << std::endl;
        std::wcout << L"Total reloads: " << stats.totalReloads << std::endl;
        std::wcout << L"Successful: " << stats.successfulReloads << std::endl;
        std::wcout << L"Failed: " << stats.failedReloads << std::endl;
        std::wcout << L"Retried: " << stats.retriedReloads << std::endl;
        std::wcout << L"Average reload time: ";
        
        if (stats.successfulReloads > 0) {
            auto avgTime = stats.totalReloadTime.count() / stats.successfulReloads;
            std::wcout << avgTime << L"ms" << std::endl;
        } else {
            std::wcout << L"N/A" << std::endl;
        }
        
        if (!stats.recentErrors.empty()) {
            std::wcout << L"\nRecent errors:" << std::endl;
            for (const auto& error : stats.recentErrors) {
                std::wcout << L"  - " << StringToWString(error) << std::endl;
            }
        }
        
        std::wcout << L"=============================" << std::endl;
    }
    
    // 현재 대기 중인 리로드 목록
    std::vector<std::string> GetPendingReloads() const {
        std::lock_guard<std::mutex> lock(pendingReloadsMutex);
        
        std::vector<std::string> pending;
        for (const auto& [filename, reload] : pendingReloads) {
            pending.push_back(filename);
        }
        return pending;
    }
    
    // 강제 리로드 (디바운싱 무시)
    bool ForceReload(const std::string& filename) {
        if (!reloadCallback) {
            LogError("No reload callback set");
            return false;
        }
        
        std::string fullPath = (fs::path(watchDirectory) / filename).string();
        
        if (!fs::exists(fullPath)) {
            LogError("File not found: " + fullPath);
            return false;
        }
        
        Log("Force reloading: " + filename);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 백업 생성 (정책에 따라)
        if (policy.backupBeforeReload) {
            CreateBackup(filename);
        }
        
        // 리로드 실행
        bool success = reloadCallback(fullPath);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 통계 업데이트
        stats.totalReloads++;
        stats.totalReloadTime += duration;
        stats.lastReloadTime = std::chrono::system_clock::now();
        
        if (success) {
            stats.successfulReloads++;
            Log("Force reload successful: " + filename + " (" + std::to_string(duration.count()) + "ms)");
            
            if (statusCallback) {
                statusCallback(filename, true);
            }
        } else {
            stats.failedReloads++;
            stats.AddError("Force reload failed: " + filename);
            LogError("Force reload failed: " + filename);
            
            if (statusCallback) {
                statusCallback(filename, false);
            }
        }
        
        return success;
    }

private:
    void WatchThreadFunction() {
        Log("File watch thread started");
        
        char buffer[4096];
        DWORD bytesReturned;
        OVERLAPPED overlapped = {};
        overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        
        while (state != HotReloadState::Stopped) {
            BOOL result = ReadDirectoryChangesW(
                hDirectory,
                buffer,
                sizeof(buffer),
                TRUE,  // Watch subdirectories
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | 
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned,
                &overlapped,
                nullptr
            );
            
            if (!result) {
                DWORD error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    LogError("ReadDirectoryChangesW failed (Error " + std::to_string(error) + ")");
                    state = HotReloadState::Error;
                    break;
                }
            }
            
            // 이벤트 대기 (타임아웃 포함)
            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, 1000);
            
            if (waitResult == WAIT_OBJECT_0) {
                // 파일 변경 이벤트 처리
                ProcessFileChangeNotifications(buffer, bytesReturned);
            } else if (waitResult == WAIT_TIMEOUT) {
                // 타임아웃 - 정상적인 상황
                continue;
            } else {
                // 오류 발생
                LogError("Wait for file change event failed");
                break;
            }
        }
        
        CloseHandle(overlapped.hEvent);
        Log("File watch thread stopped");
    }
    
    void ProcessFileChangeNotifications(char* buffer, DWORD bytesReturned) {
        auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
        
        do {
            // 파일명 변환
            std::wstring wFileName(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::string fileName = WStringToString(wFileName);
            
            // 필터링 검사
            if (ShouldProcessFile(fileName)) {
                // 변경 타입 매핑
                FileChangeType changeType = FileChangeType::Modified;
                switch (info->Action) {
                    case FILE_ACTION_ADDED:
                        changeType = FileChangeType::Created;
                        break;
                    case FILE_ACTION_REMOVED:
                        changeType = FileChangeType::Deleted;
                        break;
                    case FILE_ACTION_MODIFIED:
                        changeType = FileChangeType::Modified;
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        changeType = FileChangeType::Renamed;
                        break;
                }
                
                std::string fullPath = (fs::path(watchDirectory) / fileName).string();
                
                // 이벤트 큐에 추가
                {
                    std::lock_guard<std::mutex> lock(eventQueueMutex);
                    eventQueue.emplace(fileName, fullPath, changeType);
                }
                eventQueueCondition.notify_one();
                
                Log("File change detected: " + fileName + " (" + GetChangeTypeString(changeType) + ")");
            }
            
            // 다음 엔트리로 이동
            if (info->NextEntryOffset == 0) break;
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<char*>(info) + info->NextEntryOffset);
                
        } while (true);
    }
    
    void ReloadThreadFunction() {
        Log("Reload processing thread started");
        
        while (state != HotReloadState::Stopped) {
            std::unique_lock<std::mutex> lock(eventQueueMutex);
            
            // 이벤트 대기
            eventQueueCondition.wait(lock, [this] {
                return !eventQueue.empty() || state == HotReloadState::Stopped;
            });
            
            if (state == HotReloadState::Stopped) {
                break;
            }
            
            // 이벤트 처리
            while (!eventQueue.empty() && state == HotReloadState::Running) {
                FileChangeEvent event = eventQueue.front();
                eventQueue.pop();
                lock.unlock();
                
                ProcessFileChangeEvent(event);
                
                lock.lock();
            }
        }
        
        Log("Reload processing thread stopped");
    }
    
    void ProcessFileChangeEvent(const FileChangeEvent& event) {
        // 일시정지 상태 확인
        if (state == HotReloadState::Paused) {
            return;
        }
        
        // 삭제된 파일은 리로드하지 않음
        if (event.changeType == FileChangeType::Deleted) {
            RemovePendingReload(event.filename);
            return;
        }
        
        // 디바운싱 검사
        if (!ShouldReloadFile(event.filename, event.timestamp)) {
            return;
        }
        
        // 자동 리로드 정책 확인
        if (!policy.autoReload) {
            Log("Auto-reload disabled, file change ignored: " + event.filename);
            return;
        }
        
        // 대기 중인 리로드에 추가 또는 업데이트
        {
            std::lock_guard<std::mutex> lock(pendingReloadsMutex);
            
            auto it = pendingReloads.find(event.filename);
            if (it != pendingReloads.end()) {
                // 기존 대기 중인 리로드 업데이트
                it->second.scheduleTime = std::chrono::system_clock::now() + 
                                        std::chrono::milliseconds(policy.debounceDelayMs);
            } else {
                // 새로운 대기 리로드 추가
                pendingReloads[event.filename] = PendingReload(event.filename, event.fullPath);
                pendingReloads[event.filename].scheduleTime = std::chrono::system_clock::now() + 
                                                            std::chrono::milliseconds(policy.debounceDelayMs);
            }
        }
        
        // 디바운스 지연 후 리로드 실행
        std::thread([this, event]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(policy.debounceDelayMs));
            ExecutePendingReload(event.filename);
        }).detach();
    }
    
    void ExecutePendingReload(const std::string& filename) {
        PendingReload reload;
        
        // 대기 중인 리로드 가져오기
        {
            std::lock_guard<std::mutex> lock(pendingReloadsMutex);
            auto it = pendingReloads.find(filename);
            if (it != pendingReloads.end()) {
                return; // 이미 처리됨 또는 취소됨
            }
            
            reload = it->second;
            
            // 스케줄 시간 확인
            if (std::chrono::system_clock::now() < reload.scheduleTime) {
                return; // 아직 시간이 안 됨
            }
            
            pendingReloads.erase(it);
        }
        
        // 파일 존재 확인
        if (!fs::exists(reload.fullPath)) {
            LogError("File no longer exists: " + reload.fullPath);
            return;
        }
        
        // 리로드 실행
        bool success = PerformReload(reload);
        
        // 실패 시 재시도
        if (!success && reload.retryCount < policy.maxRetries) {
            reload.retryCount++;
            reload.scheduleTime = std::chrono::system_clock::now() + 
                                std::chrono::milliseconds(policy.debounceDelayMs * 2);
            
            {
                std::lock_guard<std::mutex> lock(pendingReloadsMutex);
                pendingReloads[filename] = reload;
            }
            
            stats.retriedReloads++;
            Log("Retrying reload (" + std::to_string(reload.retryCount) + "/" + 
                std::to_string(policy.maxRetries) + "): " + filename);
            
            // 재시도 스케줄
            std::thread([this, filename]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(policy.debounceDelayMs * 2));
                ExecutePendingReload(filename);
            }).detach();
        }
    }
    
    bool PerformReload(const PendingReload& reload) {
        if (!reloadCallback) {
            LogError("No reload callback set");
            return false;
        }
        
        Log("Reloading: " + reload.filename);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 백업 생성 (정책에 따라)
        if (policy.backupBeforeReload) {
            CreateBackup(reload.filename);
        }
        
        // 리로드 실행
        bool success = reloadCallback(reload.fullPath);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 통계 업데이트
        stats.totalReloads++;
        stats.totalReloadTime += duration;
        stats.lastReloadTime = std::chrono::system_clock::now();
        
        if (success) {
            stats.successfulReloads++;
            Log("Reload successful: " + reload.filename + " (" + std::to_string(duration.count()) + "ms)");
            
            if (statusCallback) {
                statusCallback(reload.filename, true);
            }
        } else {
            stats.failedReloads++;
            stats.AddError("Reload failed: " + reload.filename);
            LogError("Reload failed: " + reload.filename);
            
            if (statusCallback) {
                statusCallback(reload.filename, false);
            }
        }
        
        return success;
    }
    
    bool ShouldProcessFile(const std::string& filename) {
        // 확장자 검사
        std::string extension = fs::path(filename).extension().string();
        if (watchedExtensions.find(extension) == watchedExtensions.end()) {
            return false;
        }
        
        // 무시 목록 검사
        if (ignoredFiles.find(filename) != ignoredFiles.end()) {
            return false;
        }
        
        // 제외 패턴 검사
        for (const auto& pattern : policy.excludePatterns) {
            if (MatchesPattern(filename, pattern)) {
                return false;
            }
        }
        
        return true;
    }
    
    bool ShouldReloadFile(const std::string& filename, std::chrono::system_clock::time_point eventTime) {
        std::lock_guard<std::mutex> lock(modificationTimesMutex);
        
        auto it = lastModificationTimes.find(filename);
        if (it != lastModificationTimes.end()) {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                eventTime - it->second);
            
            if (timeDiff.count() < policy.debounceDelayMs / 2) {
                // 너무 빠른 연속 변경 - 무시
                return false;
            }
        }
        
        lastModificationTimes[filename] = eventTime;
        return true;
    }
    
    void RemovePendingReload(const std::string& filename) {
        std::lock_guard<std::mutex> lock(pendingReloadsMutex);
        pendingReloads.erase(filename);
    }
    
    bool MatchesPattern(const std::string& filename, const std::string& pattern) {
        // 간단한 와일드카드 패턴 매칭 (* 만 지원)
        if (pattern == "*") return true;
        
        size_t starPos = pattern.find('*');
        if (starPos == std::string::npos) {
            return filename == pattern;
        }
        
        std::string prefix = pattern.substr(0, starPos);
        std::string suffix = pattern.substr(starPos + 1);
        
        if (filename.length() < prefix.length() + suffix.length()) {
            return false;
        }
        
        return filename.substr(0, prefix.length()) == prefix &&
               filename.substr(filename.length() - suffix.length()) == suffix;
    }
    
    std::string GetChangeTypeString(FileChangeType type) {
        switch (type) {
            case FileChangeType::Modified: return "Modified";
            case FileChangeType::Created: return "Created";
            case FileChangeType::Deleted: return "Deleted";
            case FileChangeType::Renamed: return "Renamed";
            default: return "Unknown";
        }
    }
    
    void CreateBackupDirectory() {
        std::error_code ec;
        if (!fs::exists(backupDirectory)) {
            fs::create_directories(backupDirectory, ec);
            if (ec) {
                LogError("Failed to create backup directory: " + ec.message());
            }
        }
    }
    
    void Log(const std::string& message) {
        if (logCallback) {
            logCallback(message);
        }
    }
    
    void LogError(const std::string& message) {
        if (logCallback) {
            logCallback("ERROR: " + message);
        }
    }
};

// 간단한 모드 인터페이스 (테스트용)
class ITestMod {
public:
    virtual ~ITestMod() = default;
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetName() const = 0;
    virtual const char* GetVersion() const = 0;
};

// 테스트 모드 로더 (핫 리로드와 연동)
class TestModLoader {
private:
    std::map<std::string, HMODULE> loadedMods;
    std::map<std::string, ITestMod*> modInstances;
    HotReloadSystem hotReload;
    std::string modsDirectory;
    
public:
    TestModLoader() : modsDirectory("./test_mods") {
        // 핫 리로드 콜백 설정
        hotReload.SetReloadCallback([this](const std::string& filename) {
            return ReloadMod(filename);
        });
        
        hotReload.SetStatusCallback([this](const std::string& filename, bool success) {
            std::wcout << L"Reload " << (success ? L"succeeded" : L"failed") 
                     << L" for: " << StringToWString(filename) << std::endl;
        });
    }
    
    bool Initialize() {
        if (!hotReload.Initialize(modsDirectory)) {
            return false;
        }
        
        // 기존 모드들 로드
        LoadAllMods();
        
        // 핫 리로드 시작
        return hotReload.Start();
    }
    
    void Shutdown() {
        hotReload.Stop();
        UnloadAllMods();
    }
    
    bool LoadAllMods() {
        std::error_code ec;
        if (!fs::exists(modsDirectory)) {
            fs::create_directories(modsDirectory, ec);
            if (ec) return false;
        }
        
        for (const auto& entry : fs::directory_iterator(modsDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                LoadMod(entry.path().filename().string());
            }
        }
        
        return true;
    }
    
    bool LoadMod(const std::string& filename) {
        std::string fullPath = (fs::path(modsDirectory) / filename).string();
        
        // 기존 모드 언로드
        UnloadMod(filename);
        
        // DLL 로드
        HMODULE handle = LoadLibraryA(fullPath.c_str());
        if (!handle) {
            std::wcerr << L"Failed to load mod: " << StringToWString(filename) << std::endl;
            return false;
        }
        
        // 생성 함수 가져오기
        typedef ITestMod*(*CreateModFunc)();
        auto createFunc = reinterpret_cast<CreateModFunc>(GetProcAddress(handle, "CreateMod"));
        
        if (!createFunc) {
            std::wcerr << L"CreateMod function not found in: " << StringToWString(filename) << std::endl;
            FreeLibrary(handle);
            return false;
        }
        
        // 모드 인스턴스 생성
        ITestMod* mod = createFunc();
        if (!mod) {
            std::wcerr << L"Failed to create mod instance: " << StringToWString(filename) << std::endl;
            FreeLibrary(handle);
            return false;
        }
        
        // 초기화
        if (!mod->Initialize()) {
            std::wcerr << L"Mod initialization failed: " << StringToWString(filename) << std::endl;
            delete mod;
            FreeLibrary(handle);
            return false;
        }
        
        // 등록
        loadedMods[filename] = handle;
        modInstances[filename] = mod;
        
        std::wcout << L"Loaded mod: " << StringToWString(mod->GetName()) << L" v" << StringToWString(mod->GetVersion()) 
                 << L" from " << StringToWString(filename) << std::endl;
        
        return true;
    }
    
    bool ReloadMod(const std::string& fullPath) {
        std::string filename = fs::path(fullPath).filename().string();
        
        std::wcout << L"Hot reloading mod: " << StringToWString(filename) << std::endl;
        
        return LoadMod(filename);
    }
    
    void UnloadMod(const std::string& filename) {
        auto modIt = modInstances.find(filename);
        if (modIt != modInstances.end()) {
            modIt->second->Shutdown();
            delete modIt->second;
            modInstances.erase(modIt);
        }
        
        auto handleIt = loadedMods.find(filename);
        if (handleIt != loadedMods.end()) {
            FreeLibrary(handleIt->second);
            loadedMods.erase(handleIt);
        }
    }
    
    void UnloadAllMods() {
        for (auto& [filename, mod] : modInstances) {
            mod->Shutdown();
            delete mod;
        }
        modInstances.clear();
        
        for (auto& [filename, handle] : loadedMods) {
            FreeLibrary(handle);
        }
        loadedMods.clear();
    }
    
    void UpdateMods() {
        for (auto& [filename, mod] : modInstances) {
            mod->Update();
        }
    }
    
    HotReloadSystem& GetHotReload() {
        return hotReload;
    }
    
    void PrintLoadedMods() const {
        std::wcout << L"\n=== Loaded Mods ===" << std::endl;
        for (const auto& [filename, mod] : modInstances) {
            std::wcout << L"- " << StringToWString(mod->GetName()) << L" v" << StringToWString(mod->GetVersion()) 
                     << L" (" << StringToWString(filename) << L")" << std::endl;
        }
        std::wcout << L"===================" << std::endl;
    }
};

// 간단한 테스트 모드 구현
class SimpleTestMod : public ITestMod {
private:
    std::string name;
    std::string version;
    int updateCount = 0;
    
public:
    SimpleTestMod(const std::string& modName = "SimpleTestMod", 
                  const std::string& modVersion = "1.0.0")
        : name(modName), version(modVersion) {}
    
    bool Initialize() override {
        std::wcout << StringToWString(name) << L": Initialized" << std::endl;
        return true;
    }
    
    void Update() override {
        updateCount++;
        if (updateCount % 100 == 0) {
            std::wcout << StringToWString(name) << L": Update " << updateCount << std::endl;
        }
    }
    
    void Shutdown() override {
        std::wcout << StringToWString(name) << L": Shutdown (updates: " << updateCount << L")" << std::endl;
    }
    
    const char* GetName() const override {
        return name.c_str();
    }
    
    const char* GetVersion() const override {
        return version.c_str();
    }
};

// 콘솔 인터페이스
class HotReloadConsole {
private:
    TestModLoader loader;
    bool running = false;
    
public:
    void Run() {
        std::wcout << L"=== Hot Reload System Console ===" << std::endl;
        std::wcout << L"Type 'help' for available commands" << std::endl;
        
        if (!loader.Initialize()) {
            std::wcerr << L"Failed to initialize mod loader!" << std::endl;
            return;
        }
        
        running = true;
        std::wstring inputW;
        
        // 백그라운드에서 모드 업데이트
        std::thread updateThread([this]() {
            while (running) {
                loader.UpdateMods();
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            }
        });
        
        while (running) {
            std::wcout << L"\nhot_reload> ";
            std::getline(std::wcin, inputW);
            std::string input = WStringToString(inputW);
            
            ProcessCommand(input);
        }
        
        updateThread.join();
        loader.Shutdown();
    }
    
private:
    void ProcessCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        
        if (command == "help") {
            ShowHelp();
        } else if (command == "list") {
            loader.PrintLoadedMods();
        } else if (command == "reload") {
            std::string filename;
            iss >> filename;
            if (!filename.empty()) {
                if (loader.GetHotReload().TriggerReload(filename)) {
                    std::wcout << L"Reload triggered for: " << StringToWString(filename) << std::endl;
                } else {
                    std::wcout << L"Failed to trigger reload for: " << StringToWString(filename) << std::endl;
                }
            } else {
                std::wcout << L"Usage: reload <filename>" << std::endl;
            }
        } else if (command == "force") {
            std::string filename;
            iss >> filename;
            if (!filename.empty()) {
                if (loader.GetHotReload().ForceReload(filename)) {
                    std::wcout << L"Force reload completed for: " << StringToWString(filename) << std::endl;
                } else {
                    std::wcout << L"Force reload failed for: " << StringToWString(filename) << std::endl;
                }
            } else {
                std::wcout << L"Usage: force <filename>" << std::endl;
            }
        } else if (command == "stats") {
            loader.GetHotReload().PrintStatistics();
        } else if (command == "pending") {
            auto pending = loader.GetHotReload().GetPendingReloads();
            std::wcout << L"Pending reloads (" << pending.size() << L"):" << std::endl;
            for (const auto& filename : pending) {
                std::wcout << L"  - " << StringToWString(filename) << std::endl;
            }
        } else if (command == "pause") {
            loader.GetHotReload().Pause();
            std::wcout << L"Hot reload paused" << std::endl;
        } else if (command == "resume") {
            loader.GetHotReload().Resume();
            std::wcout << L"Hot reload resumed" << std::endl;
        } else if (command == "status") {
            auto state = loader.GetHotReload().GetState();
            std::wcout << L"Hot reload state: " << StringToWString(GetStateString(state)) << std::endl;
        } else if (command == "backup") {
            std::string filename;
            iss >> filename;
            if (!filename.empty()) {
                if (loader.GetHotReload().CreateBackup(filename)) {
                    std::wcout << L"Backup created for: " << StringToWString(filename) << std::endl;
                } else {
                    std::wcout << L"Failed to create backup for: " << StringToWString(filename) << std::endl;
                }
            } else {
                std::wcout << L"Usage: backup <filename>" << std::endl;
            }
        } else if (command == "restore") {
            std::string filename;
            iss >> filename;
            if (!filename.empty()) {
                if (loader.GetHotReload().RestoreBackup(filename)) {
                    std::wcout << L"Backup restored for: " << StringToWString(filename) << std::endl;
                } else {
                    std::wcout << L"Failed to restore backup for: " << StringToWString(filename) << std::endl;
                }
            } else {
                std::wcout << L"Usage: restore <filename>" << std::endl;
            }
        } else if (command == "cleanup") {
            loader.GetHotReload().CleanupOldBackups();
            std::wcout << L"Old backups cleaned up" << std::endl;
        } else if (command == "reset") {
            loader.GetHotReload().ResetStatistics();
            std::wcout << L"Statistics reset" << std::endl;
        } else if (command == "quit" || command == "exit") {
            running = false;
            std::wcout << L"Shutting down..." << std::endl;
        } else if (command.empty()) {
            // 빈 입력 무시
        } else {
            std::wcout << L"Unknown command: " << StringToWString(command) << std::endl;
            std::wcout << L"Type 'help' for available commands" << std::endl;
        }
    }
    
    void ShowHelp() {
        std::wcout << L"\nAvailable commands:" << std::endl;
        std::wcout << L"  help              - Show this help message" << std::endl;
        std::wcout << L"  list              - List all loaded mods" << std::endl;
        std::wcout << L"  reload <file>     - Trigger manual reload" << std::endl;
        std::wcout << L"  force <file>      - Force immediate reload" << std::endl;
        std::wcout << L"  stats             - Show reload statistics" << std::endl;
        std::wcout << L"  pending           - Show pending reloads" << std::endl;
        std::wcout << L"  pause             - Pause hot reload system" << std::endl;
        std::wcout << L"  resume            - Resume hot reload system" << std::endl;
        std::wcout << L"  status            - Show system status" << std::endl;
        std::wcout << L"  backup <file>     - Create backup of file" << std::endl;
        std::wcout << L"  restore <file>    - Restore from backup" << std::endl;
        std::wcout << L"  cleanup           - Clean up old backups" << std::endl;
        std::wcout << L"  reset             - Reset statistics" << std::endl;
        std::wcout << L"  quit/exit         - Exit the program" << std::endl;
    }
    
    std::string GetStateString(HotReloadState state) {
        switch (state) {
            case HotReloadState::Stopped: return "Stopped";
            case HotReloadState::Running: return "Running";
            case HotReloadState::Paused: return "Paused";
            case HotReloadState::Error: return "Error";
            default: return "Unknown";
        }
    }
};

// C 스타일 익스포트 매크로 (테스트 모드용)
#define EXPORT_TEST_MOD(className) \
    extern "C" __declspec(dllexport) ITestMod* CreateMod() { \ 
        return new className(); \ 
    } \ 
    extern "C" __declspec(dllexport) void DestroyMod(ITestMod* mod) { \ 
        delete mod; \ 
    }

// 메인 함수
int main() {
    try {
        HotReloadConsole console;
        console.Run();
    } catch (const std::exception& e) {
        std::wcerr << L"Fatal error: " << StringToWString(e.what()) << std::endl;
        return 1;
    }
    
    return 0;
}