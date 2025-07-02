/*
 * Exercise 3: 설정 관리 시스템
 * 
 * 문제: 모드별 설정을 INI 파일로 저장/로드하는 시스템을 만드세요.
 * 
 * 학습 목표:
 * - 설정 파일 파싱
 * - 타입 안전성
 * - 실시간 설정 변경
 */

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <functional>
#include <regex>
#include <codecvt>
#include <locale>
#include <limits> // For std::numeric_limits

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

// 설정 값 타입
using ConfigValue = std::variant<bool, int, float, double, std::string>;

// 설정 변경 콜백
using ConfigChangeCallback = std::function<void(const std::string& section, const std::string& key, const ConfigValue& oldValue, const ConfigValue& newValue)>;

// 설정 유효성 검사 함수
using ConfigValidator = std::function<bool(const ConfigValue& value)>;

// 설정 메타데이터
struct ConfigMetadata {
    std::string description;
    ConfigValue defaultValue;
    ConfigValue minValue;
    ConfigValue maxValue;
    ConfigValidator validator;
    bool isReadOnly = false;
    bool requiresRestart = false;
    std::vector<std::string> allowedValues;  // enum 타입 값들
    
    ConfigMetadata() = default;
    
    ConfigMetadata(const std::string& desc, const ConfigValue& defaultVal) 
        : description(desc), defaultValue(defaultVal) {}
    
    ConfigMetadata(const std::string& desc, const ConfigValue& defaultVal, 
                  const ConfigValue& minVal, const ConfigValue& maxVal)
        : description(desc), defaultValue(defaultVal), minValue(minVal), maxValue(maxVal) {}
};

class ConfigurationSystem {
private:
    // 설정 데이터: [section][key] = value
    std::map<std::string, std::map<std::string, ConfigValue>> configData;
    
    // 메타데이터: [section][key] = metadata
    std::map<std::string, std::map<std::string, ConfigMetadata>> metadata;
    
    // 변경 콜백: [section][key] = callbacks
    std::map<std::string, std::map<std::string, std::vector<ConfigChangeCallback>>> changeCallbacks;
    
    // 파일 감시
    std::map<std::string, fs::file_time_type> fileWatchList;
    std::thread fileWatchThread;
    std::atomic<bool> watchThreadRunning{false};
    
    // 스레드 안전성
    mutable std::recursive_mutex configMutex;
    
    // 설정 파일 경로
    std::string configDirectory;
    std::string globalConfigFile;
    
    // 로깅
    std::function<void(const std::string&)> logFunction;

public:
    ConfigurationSystem() : configDirectory("./config"), globalConfigFile("global.ini") {
        // 기본 로그 함수
        logFunction = [](const std::string& msg) {
            std::wcout << StringToWString("[CONFIG] " + msg) << std::endl;
        };
        
        // 설정 디렉토리 생성
        CreateConfigDirectory();
        
        // 글로벌 설정 로드
        LoadGlobalConfig();
        
        // 파일 감시 시작
        StartFileWatcher();
    }
    
    ~ConfigurationSystem() {
        StopFileWatcher();
        SaveAllConfigs();
    }
    
    // 설정 값 읽기
    template<typename T>
    T GetValue(const std::string& section, const std::string& key, const T& defaultValue = T{}) const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        auto sectionIt = configData.find(section);
        if (sectionIt != configData.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                try {
                    return std::get<T>(keyIt->second);
                } catch (const std::bad_variant_access& e) {
                    LogError("Type mismatch for config value: " + section + "." + key);
                    return defaultValue;
                }
            }
        }
        
        // 기본값 설정
        if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> || 
                     std::is_same_v<T, float> || std::is_same_v<T, double> || 
                     std::is_same_v<T, std::string>) {
            const_cast<ConfigurationSystem*>(this)->SetValue(section, key, defaultValue);
        }
        
        return defaultValue;
    }
    
    // 설정 값 쓰기
    template<typename T>
    bool SetValue(const std::string& section, const std::string& key, const T& value) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        // 읽기 전용 확인
        if (IsReadOnly(section, key)) {
            LogError("Attempt to modify read-only config: " + section + "." + key);
            return false;
        }
        
        // 유효성 검사
        ConfigValue newValue = value;
        if (!ValidateValue(section, key, newValue)) {
            LogError("Validation failed for config: " + section + "." + key);
            return false;
        }
        
        // 기존 값 가져오기
        ConfigValue oldValue;
        bool hasOldValue = false;
        
        auto sectionIt = configData.find(section);
        if (sectionIt != configData.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                oldValue = keyIt->second;
                hasOldValue = true;
            }
        }
        
        // 값 설정
        configData[section][key] = newValue;
        
        // 콜백 호출
        if (hasOldValue) {
            NotifyChange(section, key, oldValue, newValue);
        }
        
        // 자동 저장 (선택적)
        if (GetAutoSave()) {
            SaveConfig(section);
        }
        
        return true;
    }
    
    // 메타데이터 설정
    void SetMetadata(const std::string& section, const std::string& key, const ConfigMetadata& meta) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        metadata[section][key] = meta;
        
        // 기본값 설정
        if (!HasValue(section, key)) {
            configData[section][key] = meta.defaultValue;
        }
    }
    
    // 설정 스키마 정의
    void DefineSchema(const std::string& section, const std::map<std::string, ConfigMetadata>& schemaDef) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        for (const auto& [key, meta] : schemaDef) {
            SetMetadata(section, key, meta);
        }
        
        LogInfo("Schema defined for section: " + section);
    }
    
    // 콜백 등록
    void RegisterChangeCallback(const std::string& section, const std::string& key, ConfigChangeCallback callback) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        changeCallbacks[section][key].push_back(callback);
    }
    
    // 글로벌 콜백 등록 (모든 변경사항)
    void RegisterGlobalCallback(ConfigChangeCallback callback) {
        RegisterChangeCallback("*", "*", callback);
    }
    
    // 파일 I/O
    bool LoadConfig(const std::string& filename) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        fs::path filePath = fs::path(configDirectory) / filename;
        
        if (!fs::exists(filePath)) {
            LogWarning("Config file not found: " + filePath.string());
            return false;
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LogError("Failed to open config file: " + filePath.string());
            return false;
        }
        
        std::string currentSection = "General";
        std::string line;
        int lineNumber = 0;
        
        while (std::getline(file, line)) {
            lineNumber++;
            
            // 공백 및 주석 처리
            line = TrimString(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }
            
            // 섹션 처리
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }
            
            // 키=값 처리
            auto equalPos = line.find('=');
            if (equalPos == std::string::npos) {
                LogWarning("Invalid line " + std::to_string(lineNumber) + " in " + filename + ": " + line);
                continue;
            }
            
            std::string key = TrimString(line.substr(0, equalPos));
            std::string valueStr = TrimString(line.substr(equalPos + 1));
            
            // 값 파싱 및 설정
            ConfigValue value = ParseValue(valueStr);
            
            // 메타데이터 확인 후 설정
            if (HasMetadata(currentSection, key)) {
                if (!ValidateValue(currentSection, key, value)) {
                    LogWarning("Invalid value for " + currentSection + "." + key + ": " + valueStr);
                    value = GetMetadata(currentSection, key).defaultValue;
                }
            }
            
            configData[currentSection][key] = value;
        }
        
        file.close();
        
        // 파일 감시 목록에 추가
        fileWatchList[filePath.string()] = fs::last_write_time(filePath);
        
        LogInfo("Loaded config file: " + filename);
        return true;
    }
    
    bool SaveConfig(const std::string& section, const std::string& filename = "") {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        std::string actualFilename = filename;
        if (actualFilename.empty()) {
            actualFilename = section + ".ini";
        }
        
        fs::path filePath = fs::path(configDirectory) / actualFilename;
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            LogError("Failed to create config file: " + filePath.string());
            return false;
        }
        
        // 헤더 작성
        file << "# Configuration file for section: " << section << "\n";
        file << "# Generated automatically - do not edit while application is running\n";
        file << "# Last modified: " << GetCurrentTimeString() << "\n\n";
        
        // 섹션 데이터 작성
        auto sectionIt = configData.find(section);
        if (sectionIt != configData.end()) {
            file << "[" << section << "]\n";
            
            for (const auto& [key, value] : sectionIt->second) {
                // 메타데이터가 있으면 주석으로 설명 추가
                if (HasMetadata(section, key)) {
                    const auto& meta = GetMetadata(section, key);
                    if (!meta.description.empty()) {
                        file << "# " << meta.description << "\n";
                    }
                    if (meta.isReadOnly) {
                        file << "# READ-ONLY\n";
                    }
                    if (meta.requiresRestart) {
                        file << "# Requires restart to take effect\n";
                    }
                    if (!meta.allowedValues.empty()) {
                        file << "# Allowed values: ";
                        for (size_t i = 0; i < meta.allowedValues.size(); ++i) {
                            if (i > 0) file << ", ";
                            file << meta.allowedValues[i];
                        }
                        file << "\n";
                    }
                }
                
                file << key << "=" << ValueToString(value) << "\n\n";
            }
        }
        
        file.close();
        
        LogInfo("Saved config file: " + actualFilename);
        return true;
    }
    
    void SaveAllConfigs() {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        for (const auto& [section, _] : configData) {
            SaveConfig(section);
        }
    }
    
    // 유틸리티 함수들
    bool HasValue(const std::string& section, const std::string& key) const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        auto sectionIt = configData.find(section);
        if (sectionIt != configData.end()) {
            return sectionIt->second.find(key) != sectionIt->second.end();
        }
        return false;
    }
    
    bool HasSection(const std::string& section) const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        return configData.find(section) != configData.end();
    }
    
    std::vector<std::string> GetSections() const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        std::vector<std::string> sections;
        for (const auto& [section, _] : configData) {
            sections.push_back(section);
        }
        return sections;
    }
    
    std::vector<std::string> GetKeys(const std::string& section) const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        std::vector<std::string> keys;
        auto sectionIt = configData.find(section);
        if (sectionIt != configData.end()) {
            for (const auto& [key, _] : sectionIt->second) {
                keys.push_back(key);
            }
        }
        return keys;
    }
    
    void RemoveValue(const std::string& section, const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        auto sectionIt = configData.find(section);
        if (sectionIt != configData.end()) {
            sectionIt->second.erase(key);
        }
    }
    
    void RemoveSection(const std::string& section) {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        configData.erase(section);
        metadata.erase(section);
        changeCallbacks.erase(section);
    }
    
    // 설정 내보내기/가져오기
    bool ExportToJSON(const std::string& filename) const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        
        file << "{\n";
        bool firstSection = true;
        
        for (const auto& [section, keys] : configData) {
            if (!firstSection) file << ",\n";
            firstSection = false;
            
            file << "  \"" << section << "\": {\n";
            bool firstKey = true;
            
            for (const auto& [key, value] : keys) {
                if (!firstKey) file << ",\n";
                firstKey = false;
                
                file << "    \"" << key << "\": " << ValueToJSON(value);
            }
            
            file << "\n  }";
        }
        
        file << "\n}\n";
        file.close();
        
        return true;
    }
    
    // 설정 검증 및 복구
    bool ValidateAllConfigs() {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        bool allValid = true;
        
        for (auto& [section, keys] : configData) {
            for (auto& [key, value] : keys) {
                if (!ValidateValue(section, key, value)) {
                    LogWarning("Invalid config value found: " + section + "." + key);
                    
                    // 기본값으로 복구
                    if (HasMetadata(section, key)) {
                        value = GetMetadata(section, key).defaultValue;
                        LogInfo("Restored default value for: " + section + "." + key);
                    }
                    
                    allValid = false;
                }
            }
        }
        
        return allValid;
    }
    
    // 설정 통계
    struct ConfigStats {
        int totalSections = 0;
        int totalKeys = 0;
        int readOnlyKeys = 0;
        int keysWithCallbacks = 0;
        size_t memoryUsage = 0;
    };
    
    ConfigStats GetStatistics() const {
        std::lock_guard<std::recursive_mutex> lock(configMutex);
        
        ConfigStats stats;
        stats.totalSections = static_cast<int>(configData.size());
        
        for (const auto& [section, keys] : configData) {
            stats.totalKeys += static_cast<int>(keys.size());
        }
        
        for (const auto& [section, sectionMeta] : metadata) {
            for (const auto& [key, meta] : sectionMeta) {
                if (meta.isReadOnly) stats.readOnlyKeys++;
            }
        }
        
        for (const auto& [section, sectionCallbacks] : changeCallbacks) {
            for (const auto& [key, callbacks] : sectionCallbacks) {
                if (!callbacks.empty()) stats.keysWithCallbacks++;
            }
        }
        
        // 대략적인 메모리 사용량 계산
        stats.memoryUsage = sizeof(*this);
        
        return stats;
    }
    
    void PrintStatistics() const {
        auto stats = GetStatistics();
        
        std::wcout << L"\n=== Configuration Statistics ===" << std::endl;
        std::wcout << L"Total sections: " << stats.totalSections << std::endl;
        std::wcout << L"Total keys: " << stats.totalKeys << std::endl;
        std::wcout << L"Read-only keys: " << stats.readOnlyKeys << std::endl;
        std::wcout << L"Keys with callbacks: " << stats.keysWithCallbacks << std::endl;
        std::wcout << L"Estimated memory usage: " << stats.memoryUsage << L" bytes" << std::endl;
        std::wcout << L"===============================" << std::endl;
    }
    
    // 설정 백업 및 복원
    bool CreateBackup(const std::string& backupName = "") {
        std::string actualBackupName = backupName;
        if (actualBackupName.empty()) {
            actualBackupName = "backup_" + GetCurrentTimeString();
        }
        
        fs::path backupDir = fs::path(configDirectory) / "backups" / actualBackupName;
        
        try {
            fs::create_directories(backupDir);
            
            for (const auto& [section, _] : configData) {
                fs::path sourceFile = fs::path(configDirectory) / (section + ".ini");
                fs::path destFile = backupDir / (section + ".ini");
                
                if (fs::exists(sourceFile)) {
                    fs::copy_file(sourceFile, destFile);
                }
            }
            
            LogInfo("Configuration backup created: " + actualBackupName);
            return true;
        } catch (const fs::filesystem_error& e) {
            LogError("Failed to create backup: " + std::string(e.what()));
            return false;
        }
    }
    
    std::vector<std::string> GetAvailableBackups() const {
        std::vector<std::string> backups;
        fs::path backupDir = fs::path(configDirectory) / "backups";
        
        if (fs::exists(backupDir) && fs::is_directory(backupDir)) {
            for (const auto& entry : fs::directory_iterator(backupDir)) {
                if (entry.is_directory()) {
                    backups.push_back(entry.path().filename().string());
                }
            }
        }
        
        return backups;
    }

private:
    // 파일 감시 시스템
    void StartFileWatcher() {
        watchThreadRunning = true;
        fileWatchThread = std::thread(&ConfigurationSystem::FileWatcherThread, this);
    }
    
    void StopFileWatcher() {
        watchThreadRunning = false;
        if (fileWatchThread.joinable()) {
            fileWatchThread.join();
        }
    }
    
    void FileWatcherThread() {
        while (watchThreadRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            std::lock_guard<std::recursive_mutex> lock(configMutex);
            
            for (auto& [filename, lastWriteTime] : fileWatchList) {
                if (fs::exists(filename)) {
                    auto currentWriteTime = fs::last_write_time(filename);
                    if (currentWriteTime > lastWriteTime) {
                        LogInfo("Config file changed, reloading: " + filename);
                        
                        // 파일 재로드
                        std::string configFile = fs::path(filename).filename().string();
                        LoadConfig(configFile);
                        
                        lastWriteTime = currentWriteTime;
                    }
                }
            }
        }
    }
    
    // 유틸리티 함수들
    void CreateConfigDirectory() {
        try {
            fs::create_directories(configDirectory);
            fs::create_directories(fs::path(configDirectory) / "backups");
        } catch (const fs::filesystem_error& e) {
            LogError("Failed to create config directory: " + std::string(e.what()));
        }
    }
    
    void LoadGlobalConfig() {
        // 글로벌 설정 스키마 정의
        DefineSchema("System", {
            {"auto_save", ConfigMetadata("Automatically save configuration changes", true)},
            {"backup_on_start", ConfigMetadata("Create backup on startup", true)},
            {"file_watch_enabled", ConfigMetadata("Enable automatic file watching", true)},
            {"log_level", ConfigMetadata("Logging level (0=Debug, 1=Info, 2=Warning, 3=Error)", 1, 0, 3)},
            {"max_backups", ConfigMetadata("Maximum number of backups to keep", 10, 1, 100)}
        });
        
        LoadConfig(globalConfigFile);
    }
    
    ConfigValue ParseValue(const std::string& str) {
        std::string trimmed = TrimString(str);
        
        // Boolean 값
        if (trimmed == "true" || trimmed == "false") {
            return trimmed == "true";
        }
        
        // 정수 값
        if (std::regex_match(trimmed, std::regex("^-?\\d+$"))) {
            return std::stoi(trimmed);
        }
        
        // 실수 값
        if (std::regex_match(trimmed, std::regex("^-?\\d*\\.\\d+$"))) {
            return std::stof(trimmed);
        }
        
        // 문자열 값 (따옴표 제거)
        if ((trimmed.front() == '"' && trimmed.back() == '"') ||
            (trimmed.front() == '\'' && trimmed.back() == '\'')) {
            return trimmed.substr(1, trimmed.length() - 2);
        }
        
        // 기본값은 문자열
        return trimmed;
    }
    
    std::string ValueToString(const ConfigValue& value) {
        return std::visit([](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + v + "\"";
            } else {
                return std::to_string(v);
            }
        }, value);
    }
    
    std::string ValueToJSON(const ConfigValue& value) const {
        return std::visit([](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + v + "\"";
            } else {
                return std::to_string(v);
            }
        }, value);
    }
    
    bool ValidateValue(const std::string& section, const std::string& key, const ConfigValue& value) {
        if (!HasMetadata(section, key)) {
            return true;  // 메타데이터 없으면 유효하다고 가정
        }
        
        const auto& meta = GetMetadata(section, key);
        
        // 커스텀 검증 함수
        if (meta.validator && !meta.validator(value)) {
            return false;
        }
        
        // 허용된 값 목록 확인
        if (!meta.allowedValues.empty()) {
            std::string valueStr = ValueToString(value);
            valueStr.erase(0, valueStr.find_first_not_of("\"'"));
            valueStr.erase(valueStr.find_last_not_of("\"'") + 1);
            
            bool found = std::find(meta.allowedValues.begin(), meta.allowedValues.end(), valueStr) != meta.allowedValues.end();
            if (!found) return false;
        }
        
        // 범위 검증
        return std::visit([&meta](const auto& v) -> bool {
            using T = std::decay_t<decltype(v)>;
            
            if constexpr (std::is_arithmetic_v<T>) {
                bool hasMin = std::holds_alternative<T>(meta.minValue);
                bool hasMax = std::holds_alternative<T>(meta.maxValue);
                
                if (hasMin && v < std::get<T>(meta.minValue)) return false;
                if (hasMax && v > std::get<T>(meta.maxValue)) return false;
            }
            
            return true;
        }, value);
    }
    
    bool HasMetadata(const std::string& section, const std::string& key) const {
        auto sectionIt = metadata.find(section);
        if (sectionIt != metadata.end()) {
            return sectionIt->second.find(key) != sectionIt->second.end();
        }
        return false;
    }
    
    const ConfigMetadata& GetMetadata(const std::string& section, const std::string& key) const {
        static ConfigMetadata emptyMeta;
        
        auto sectionIt = metadata.find(section);
        if (sectionIt != metadata.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                return keyIt->second;
            }
        }
        return emptyMeta;
    }
    
    bool IsReadOnly(const std::string& section, const std::string& key) const {
        if (HasMetadata(section, key)) {
            return GetMetadata(section, key).isReadOnly;
        }
        return false;
    }
    
    bool GetAutoSave() const {
        return GetValue<bool>("System", "auto_save", true);
    }
    
    void NotifyChange(const std::string& section, const std::string& key, 
                     const ConfigValue& oldValue, const ConfigValue& newValue) {
        // 특정 콜백 호출
        auto sectionIt = changeCallbacks.find(section);
        if (sectionIt != changeCallbacks.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                for (auto& callback : keyIt->second) {
                    try {
                        callback(section, key, oldValue, newValue);
                    }
                    catch (const std::exception& e) {
                        LogError("Config callback error: " + std::string(e.what()));
                    }
                }
            }
        }
        
        // 글로벌 콜백 호출
        auto globalIt = changeCallbacks.find("*");
        if (globalIt != changeCallbacks.end()) {
            auto wildcardIt = globalIt->second.find("*");
            if (wildcardIt != globalIt->second.end()) {
                for (auto& callback : wildcardIt->second) {
                    try {
                        callback(section, key, oldValue, newValue);
                    }
                    catch (const std::exception& e) {
                        LogError("Global config callback error: " + std::string(e.what()));
                    }
                }
            }
        }
    }
    
    std::string TrimString(const std::string& str) {
        const char* whitespace = " \t\r\n";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }
    
    std::string GetCurrentTimeString() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        return ss.str();
    }
    
    void LogInfo(const std::string& message) const {
        if (logFunction) {
            logFunction("[INFO] " + message);
        }
    }
    
    void LogWarning(const std::string& message) const {
        if (logFunction) {
            logFunction("[WARNING] " + message);
        }
    }
    
    void LogError(const std::string& message) const {
        if (logFunction) {
            logFunction("[ERROR] " + message);
        }
    }
};

// 설정 관리자 사용 예제
class GameSettings {
private:
    std::unique_ptr<ConfigurationSystem> config;
    
public:
    GameSettings() {
        config = std::make_unique<ConfigurationSystem>();
        InitializeSchema();
        LoadSettings();
    }
    
    void InitializeSchema() {
        // 그래픽 설정
        config->DefineSchema("Graphics", {
            {"resolution_width", ConfigMetadata("Screen width", 1920, 640, 3840)},
            {"resolution_height", ConfigMetadata("Screen height", 1080, 480, 2160)},
            {"fullscreen", ConfigMetadata("Fullscreen mode", true)},
            {"vsync", ConfigMetadata("Vertical sync", true)},
            {"anti_aliasing", ConfigMetadata("Anti-aliasing quality", std::string("Medium"))},
            {"texture_quality", ConfigMetadata("Texture quality", std::string("High"))},
            {"shadow_quality", ConfigMetadata("Shadow quality", std::string("Medium"))},
            {"render_scale", ConfigMetadata("Render scale", 1.0f, 0.5f, 2.0f)}
        });
        
        // 메타데이터 설정 (허용된 값)
        auto& antiAliasingMeta = const_cast<ConfigMetadata&>(config->GetMetadata("Graphics", "anti_aliasing"));
        antiAliasingMeta.allowedValues = {"Off", "Low", "Medium", "High", "Ultra"};
        
        auto& textureMeta = const_cast<ConfigMetadata&>(config->GetMetadata("Graphics", "texture_quality"));
        textureMeta.allowedValues = {"Low", "Medium", "High", "Ultra"};
        
        auto& shadowMeta = const_cast<ConfigMetadata&>(config->GetMetadata("Graphics", "shadow_quality"));
        shadowMeta.allowedValues = {"Off", "Low", "Medium", "High"};
        
        // 오디오 설정
        config->DefineSchema("Audio", {
            {"master_volume", ConfigMetadata("Master volume", 1.0f, 0.0f, 1.0f)},
            {"music_volume", ConfigMetadata("Music volume", 0.8f, 0.0f, 1.0f)},
            {"sfx_volume", ConfigMetadata("Sound effects volume", 0.9f, 0.0f, 1.0f)},
            {"voice_volume", ConfigMetadata("Voice volume", 1.0f, 0.0f, 1.0f)},
            {"audio_device", ConfigMetadata("Audio output device", std::string("Default"))},
            {"surround_sound", ConfigMetadata("Surround sound", false)},
            {"dynamic_range", ConfigMetadata("Dynamic range compression", false)}
        });
        
        // 게임플레이 설정
        config->DefineSchema("Gameplay", {
            {"difficulty", ConfigMetadata("Game difficulty", std::string("Normal"))},
            {"mouse_sensitivity", ConfigMetadata("Mouse sensitivity", 1.0f, 0.1f, 5.0f)},
            {"invert_mouse", ConfigMetadata("Invert mouse Y-axis", false)},
            {"auto_save", ConfigMetadata("Enable auto-save", true)},
            {"auto_save_interval", ConfigMetadata("Auto-save interval (minutes)", 5, 1, 60)},
            {"subtitles", ConfigMetadata("Enable subtitles", true)},
            {"hud_scale", ConfigMetadata("HUD scale", 1.0f, 0.5f, 2.0f)}
        });
        
        auto& difficultyMeta = const_cast<ConfigMetadata&>(config->GetMetadata("Gameplay", "difficulty"));
        difficultyMeta.allowedValues = {"Easy", "Normal", "Hard", "Nightmare"};
        
        // 키 바인딩
        config->DefineSchema("Controls", {
            {"key_forward", ConfigMetadata("Move forward key", std::string("W"))},
            {"key_backward", ConfigMetadata("Move backward key", std::string("S"))},
            {"key_left", ConfigMetadata("Move left key", std::string("A"))},
            {"key_right", ConfigMetadata("Move right key", std::string("D"))},
            {"key_jump", ConfigMetadata("Jump key", std::string("Space"))},
            {"key_crouch", ConfigMetadata("Crouch key", std::string("C"))},
            {"key_run", ConfigMetadata("Run key", std::string("Shift"))},
            {"key_interact", ConfigMetadata("Interact key", std::string("E"))},
            {"key_inventory", ConfigMetadata("Inventory key", std::string("I"))},
            {"key_menu", ConfigMetadata("Menu key", std::string("Escape"))}
        });
    }
    
    void LoadSettings() {
        config->LoadConfig("graphics.ini");
        config->LoadConfig("audio.ini");
        config->LoadConfig("gameplay.ini");
        config->LoadConfig("controls.ini");
        
        // 변경 감지 콜백 등록
        config->RegisterChangeCallback("Graphics", "resolution_width", 
            [this](const std::string& section, const std::string& key, const ConfigValue& oldVal, const ConfigValue& newVal) {
                std::wcout << L"Resolution width changed from " << std::get<int>(oldVal) 
                         << L" to " << std::get<int>(newVal) << std::endl;
                // 실제 해상도 변경 로직
            });
        
        config->RegisterChangeCallback("Audio", "master_volume",
            [this](const std::string& section, const std::string& key, const ConfigValue& oldVal, const ConfigValue& newVal) {
                std::wcout << L"Master volume changed from " << std::get<float>(oldVal)
                         << L" to " << std::get<float>(newVal) << std::endl;
                // 실제 볼륨 변경 로직
            });
    }
    
    // 편의 함수들
    void SetResolution(int width, int height) {
        config->SetValue("Graphics", "resolution_width", width);
        config->SetValue("Graphics", "resolution_height", height);
    }
    
    std::pair<int, int> GetResolution() {
        int width = config->GetValue<int>("Graphics", "resolution_width", 1920);
        int height = config->GetValue<int>("Graphics", "resolution_height", 1080);
        return {width, height};
    }
    
    void SetMasterVolume(float volume) {
        config->SetValue("Audio", "master_volume", std::clamp(volume, 0.0f, 1.0f));
    }
    
    float GetMasterVolume() {
        return config->GetValue<float>("Audio", "master_volume", 1.0f);
    }
    
    void SetDifficulty(const std::string& difficulty) {
        config->SetValue("Gameplay", "difficulty", difficulty);
    }
    
    std::string GetDifficulty() {
        return config->GetValue<std::string>("Gameplay", "difficulty", "Normal");
    }
    
    void SaveAll() {
        config->SaveAllConfigs();
    }
    
    void PrintAllSettings() {
        std::wcout << L"\n=== Current Game Settings ===" << std::endl;
        
        auto sections = config->GetSections();
        for (const auto& section : sections) {
            std::wcout << L"\n[" << StringToWString(section) << L"]" << std::endl;
            
            auto keys = config->GetKeys(section);
            for (const auto& key : keys) {
                // 값 타입에 따라 적절히 출력
                if (config->HasValue(section, key)) {
                    std::wcout << L"  " << StringToWString(key) << L" = ";
                    
                    // 타입별 출력 (간단한 버전)
                    try {
                        bool boolVal = config->GetValue<bool>(section, key);
                        std::wcout << (boolVal ? L"true" : L"false") << std::endl;
                        continue;
                    } catch (...) {}
                    
                    try {
                        int intVal = config->GetValue<int>(section, key);
                        std::wcout << intVal << std::endl;
                        continue;
                    } catch (...) {}
                    
                    try {
                        float floatVal = config->GetValue<float>(section, key);
                        std::wcout << floatVal << std::endl;
                        continue;
                    } catch (...) {}
                    
                    try {
                        std::string strVal = config->GetValue<std::string>(section, key);
                        std::wcout << L"\"" << StringToWString(strVal) << L"\"" << std::endl;
                        continue;
                    } catch (...) {}
                    
                    std::wcout << L"unknown type" << std::endl;
                }
            }
        }
        std::wcout << L"=============================" << std::endl;
    }
    
    ConfigurationSystem* GetConfigSystem() {
        return config.get();
    }
};

// 메인 테스트 프로그램
class ConfigTestProgram {
private:
    std::unique_ptr<GameSettings> gameSettings;
    bool running;
    
public:
    ConfigTestProgram() : running(false) {}
    
    void Run() {
        std::wcout << L"=== Configuration System Test ===" << std::endl;
        
        gameSettings = std::make_unique<GameSettings>();
        running = true;
        
        std::wcout << L"Type 'help' for commands, 'quit' to exit" << std::endl;
        
        std::wstring inputW;
        while (running) {
            std::wcout << L"\nconfig> ";
            std::getline(std::wcin, inputW);
            std::string input = WStringToString(inputW);
            
            ProcessCommand(input);
        }
    }
    
private:
    void ProcessCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        
        if (command == "help") {
            ShowHelp();
        } else if (command == "list") {
            gameSettings->PrintAllSettings();
        } else if (command == "stats") {
            gameSettings->GetConfigSystem()->PrintStatistics();
        } else if (command == "set") {
            std::string section, key, value;
            iss >> section >> key >> value;
            SetConfigValue(section, key, value);
        } else if (command == "get") {
            std::string section, key;
            iss >> section >> key;
            GetConfigValue(section, key);
        } else if (command == "save") {
            gameSettings->SaveAll();
            std::wcout << L"All configurations saved" << std::endl;
        } else if (command == "load") {
            std::string filename;
            iss >> filename;
            if (!filename.empty()) {
                if (gameSettings->GetConfigSystem()->LoadConfig(filename)) {
                    std::wcout << L"Loaded config file: " << StringToWString(filename) << std::endl;
                } else {
                    std::wcout << L"Failed to load config file: " << StringToWString(filename) << std::endl;
                }
            }
        } else if (command == "backup") {
            if (gameSettings->GetConfigSystem()->CreateBackup()) {
                std::wcout << L"Backup created successfully" << std::endl;
            } else {
                std::wcout << L"Failed to create backup" << std::endl;
            }
        } else if (command == "validate") {
            if (gameSettings->GetConfigSystem()->ValidateAllConfigs()) {
                std::wcout << L"All configurations are valid" << std::endl;
            } else {
                std::wcout << L"Some configurations were invalid and have been reset" << std::endl;
            }
        } else if (command == "export") {
            std::string filename;
            iss >> filename;
            if (filename.empty()) filename = "config_export.json";
            
            if (gameSettings->GetConfigSystem()->ExportToJSON(filename)) {
                std::wcout << L"Configuration exported to: " << StringToWString(filename) << std::endl;
            } else {
                std::wcout << L"Failed to export configuration" << std::endl;
            }
        } else if (command == "resolution") {
            std::string widthStr, heightStr;
            iss >> widthStr >> heightStr;
            if (!widthStr.empty() && !heightStr.empty()) {
                int width = std::stoi(widthStr);
                int height = std::stoi(heightStr);
                gameSettings->SetResolution(width, height);
                std::wcout << L"Resolution set to " << width << L"x" << height << std::endl;
            } else {
                auto [width, height] = gameSettings->GetResolution();
                std::wcout << L"Current resolution: " << width << L"x" << height << std::endl;
            }
        } else if (command == "volume") {
            std::string volumeStr;
            iss >> volumeStr;
            if (!volumeStr.empty()) {
                float volume = std::stof(volumeStr);
                gameSettings->SetMasterVolume(volume);
                std::wcout << L"Master volume set to " << volume << std::endl;
            } else {
                std::wcout << L"Current master volume: " << gameSettings->GetMasterVolume() << std::endl;
            }
        } else if (command == "quit" || command == "exit") {
            running = false;
        } else if (!command.empty()) {
            std::wcout << L"Unknown command: " << StringToWString(command) << std::endl;
        }
    }
    
    void SetConfigValue(const std::string& section, const std::string& key, const std::string& value) {
        auto* config = gameSettings->GetConfigSystem();
        
        // 값 타입 추정해서 설정
        if (value == "true" || value == "false") {
            config->SetValue(section, key, value == "true");
        } else if (value.find('.') != std::string::npos) {
            try {
                config->SetValue(section, key, std::stof(value));
            } catch (...) {
                config->SetValue(section, key, value);
            }
        } else {
            try {
                config->SetValue(section, key, std::stoi(value));
            } catch (...) {
                config->SetValue(section, key, value);
            }
        }
        
        std::wcout << L"Set " << StringToWString(section) << L"." << StringToWString(key) << L" = " << StringToWString(value) << std::endl;
    }
    
    void GetConfigValue(const std::string& section, const std::string& key) {
        auto* config = gameSettings->GetConfigSystem();
        
        if (!config->HasValue(section, key)) {
            std::wcout << L"Key not found: " << StringToWString(section) << L"." << StringToWString(key) << std::endl;
            return;
        }
        
        // 타입별 출력
        try {
            bool val = config->GetValue<bool>(section, key);
            std::wcout << StringToWString(section) << L"." << StringToWString(key) << L" = " << (val ? L"true" : L"false") << std::endl;
            return;
        } catch (...) {}
        
        try {
            int val = config->GetValue<int>(section, key);
            std::wcout << StringToWString(section) << L"." << StringToWString(key) << L" = " << val << std::endl;
            return;
        } catch (...) {}
        
        try {
            float val = config->GetValue<float>(section, key);
            std::wcout << StringToWString(section) << L"." << StringToWString(key) << L" = " << val << std::endl;
            return;
        } catch (...) {}
        
        try {
            std::string val = config->GetValue<std::string>(section, key);
            std::wcout << StringToWString(section) << L"." << StringToWString(key) << L" = \"" << StringToWString(val) << L"\"" << std::endl;
            return;
        } catch (...) {}
        
        std::wcout << StringToWString(section) << L"." << StringToWString(key) << L" = unknown type" << std::endl;
    }
    
    void ShowHelp() {
        std::wcout << L"\nAvailable commands:" << std::endl;
        std::wcout << L"  help                    - Show this help" << std::endl;
        std::wcout << L"  list                    - List all configuration values" << std::endl;
        std::wcout << L"  stats                   - Show configuration statistics" << std::endl;
        std::wcout << L"  set <section> <key> <value> - Set a configuration value" << std::endl;
        std::wcout << L"  get <section> <key>     - Get a configuration value" << std::endl;
        std::wcout << L"  save                    - Save all configurations" << std::endl;
        std::wcout << L"  load <filename>         - Load configuration file" << std::endl;
        std::wcout << L"  backup                  - Create configuration backup" << std::endl;
        std::wcout << L"  validate                - Validate all configurations" << std::endl;
        std::wcout << L"  export [filename]       - Export to JSON" << std::endl;
        std::wcout << L"  resolution [width height] - Set/get resolution" << std::endl;
        std::wcout << L"  volume [value]          - Set/get master volume" << std::endl;
        std::wcout << L"  quit/exit               - Exit program" << std::endl;
    }
};

// 메인 함수
int main() {
    try {
        ConfigTestProgram program;
        program.Run();
    } catch (const std::exception& e) {
        std::wcerr << L"Fatal error: " << StringToWString(e.what()) << std::endl;
        return 1;
    }
    
    return 0;
}