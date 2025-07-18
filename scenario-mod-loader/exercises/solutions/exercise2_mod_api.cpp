/*
 * Exercise 2: 모드 API 시스템
 * 
 * 문제: 모드들이 사용할 수 있는 공통 API 인터페이스를 설계하고 구현하세요.
 * 
 * 학습 목표:
 * - API 디자인 패턴
 * - 인터페이스 분리 원칙
 * - 모드 간 통신 시스템
 */

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <any>
#include <typeinfo>
#include <algorithm>
#include <codecvt>
#include <locale>

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

// 이벤트 시스템
struct Event {
    std::string type;
    std::unordered_map<std::string, std::any> data;
    std::chrono::system_clock::time_point timestamp;
    std::string sender;
    
    Event(const std::string& eventType, const std::string& senderMod = "") 
        : type(eventType), sender(senderMod), timestamp(std::chrono::system_clock::now()) {}
    
    template<typename T>
    void SetData(const std::string& key, const T& value) {
        data[key] = value;
    }
    
    template<typename T>
    T GetData(const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast& e) {
                throw std::runtime_error("Type mismatch for event data key: " + key);
            }
        }
        throw std::runtime_error("Event data key not found: " + key);
    }
    
    template<typename T>
    T GetData(const std::string& key, const T& defaultValue) const {
        try {
            return GetData<T>(key);
        } catch (...) {
            return defaultValue;
        }
    }
    
    bool HasData(const std::string& key) const {
        return data.find(key) != data.end();
    }
};

// 로깅 시스템 인터페이스
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void Log(LogLevel level, const std::string& message, const std::string& category = "") = 0;
    virtual void SetLogLevel(LogLevel level) = 0;
    virtual void EnableFileLogging(const std::string& filename) = 0;
    virtual void DisableFileLogging() = 0;
};

// 설정 관리 인터페이스
class IConfigManager {
public:
    virtual ~IConfigManager() = default;
    
    // 기본 타입 지원
    virtual bool GetBool(const std::string& key, bool defaultValue = false) const = 0;
    virtual int GetInt(const std::string& key, int defaultValue = 0) const = 0;
    virtual float GetFloat(const std::string& key, float defaultValue = 0.0f) const = 0;
    virtual std::string GetString(const std::string& key, const std::string& defaultValue = "") const = 0;
    
    virtual void SetBool(const std::string& key, bool value) = 0;
    virtual void SetInt(const std::string& key, int value) = 0;
    virtual void SetFloat(const std::string& key, float value) = 0;
    virtual void SetString(const std::string& key, const std::string& value) = 0;
    
    // 섹션 지원
    virtual void SetSection(const std::string& section) = 0;
    virtual std::vector<std::string> GetSections() const = 0;
    virtual std::vector<std::string> GetKeys(const std::string& section = "") const = 0;
    
    // 파일 I/O
    virtual bool LoadFromFile(const std::string& filename) = 0;
    virtual bool SaveToFile(const std::string& filename) const = 0;
    virtual bool HasKey(const std::string& key) const = 0;
    virtual void RemoveKey(const std::string& key) = 0;
};

// 입력 관리 인터페이스
class IInputManager {
public:
    virtual ~IInputManager() = default;
    
    // 키보드
    virtual bool IsKeyPressed(int virtualKey) const = 0;
    virtual bool IsKeyHeld(int virtualKey) const = 0;
    virtual bool IsKeyReleased(int virtualKey) const = 0;
    
    // 마우스
    virtual bool IsMouseButtonPressed(int button) const = 0;
    virtual bool IsMouseButtonHeld(int button) const = 0;
    virtual std::pair<int, int> GetMousePosition() const = 0;
    virtual std::pair<int, int> GetMouseDelta() const = 0;
    virtual int GetMouseWheel() const = 0;
    
    // 핫키 시스템
    virtual bool RegisterHotkey(const std::string& name, int virtualKey, int modifiers = 0) = 0;
    virtual bool UnregisterHotkey(const std::string& name) = 0;
    virtual bool IsHotkeyPressed(const std::string& name) const = 0;
};

// 메모리 관리 인터페이스
class IMemoryManager {
public:
    virtual ~IMemoryManager() = default;
    
    // 메모리 읽기/쓰기
    virtual bool ReadMemory(uintptr_t address, void* buffer, size_t size) const = 0;
    virtual bool WriteMemory(uintptr_t address, const void* data, size_t size) const = 0;
    
    template<typename T>
    bool ReadValue(uintptr_t address, T& value) const {
        return ReadMemory(address, &value, sizeof(T));
    }
    
    template<typename T>
    bool WriteValue(uintptr_t address, const T& value) const {
        return WriteMemory(address, &value, sizeof(T));
    }
    
    // 패턴 스캔
    virtual uintptr_t FindPattern(const std::string& pattern, const std::string& mask, uintptr_t startAddress = 0, uintptr_t endAddress = 0) const = 0;
    virtual std::vector<uintptr_t> FindAllPatterns(const std::string& pattern, const std::string& mask) const = 0;
    
    // 메모리 보호
    virtual bool ProtectMemory(uintptr_t address, size_t size, DWORD protection, DWORD* oldProtection = nullptr) const = 0;
    virtual bool AllocateMemory(size_t size, uintptr_t preferredAddress = 0) = 0;
    virtual bool FreeMemory(uintptr_t address) = 0;
};

// 후킹 관리 인터페이스
class IHookManager {
public:
    virtual ~IHookManager() = default;
    
    // 함수 후킹
    virtual bool InstallHook(const std::string& name, uintptr_t targetAddress, void* hookFunction, void** originalFunction) = 0;
    virtual bool UninstallHook(const std::string& name) = 0;
    virtual bool IsHookInstalled(const std::string& name) const = 0;
    
    // 핫패치
    virtual bool PatchBytes(const std::string& name, uintptr_t address, const std::vector<uint8_t>& newBytes) = 0;
    virtual bool RestorePatch(const std::string& name) = 0;
    
    // 가상 함수 후킹
    virtual bool HookVTable(const std::string& name, void* object, int index, void* hookFunction, void** originalFunction) = 0;
    virtual bool UnhookVTable(const std::string& name) = 0;
};

// 렌더링 인터페이스
class IRenderManager {
public:
    virtual ~IRenderManager() = default;
    
    // 텍스트 렌더링
    virtual void DrawText(const std::string& text, float x, float y, uint32_t color = 0xFFFFFFFF, float scale = 1.0f) = 0;
    virtual void DrawTextCentered(const std::string& text, float x, float y, uint32_t color = 0xFFFFFFFF, float scale = 1.0f) = 0;
    
    // 기본 도형
    virtual void DrawLine(float x1, float y1, float x2, float y2, uint32_t color = 0xFFFFFFFF, float thickness = 1.0f) = 0;
    virtual void DrawRect(float x, float y, float width, float height, uint32_t color = 0xFFFFFFFF, bool filled = false) = 0;
    virtual void DrawCircle(float x, float y, float radius, uint32_t color = 0xFFFFFFFF, bool filled = false) = 0;
    
    // 화면 정보
    virtual std::pair<int, int> GetScreenSize() const = 0;
    virtual void SetRenderTarget(void* renderTarget) = 0;
};

// 파일 시스템 인터페이스
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    
    // 파일 I/O
    virtual std::vector<uint8_t> ReadFile(const std::string& filename) const = 0;
    virtual bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data) const = 0;
    virtual std::string ReadTextFile(const std::string& filename) const = 0;
    virtual bool WriteTextFile(const std::string& filename, const std::string& content) const = 0;
    
    // 디렉토리 작업
    virtual std::vector<std::string> ListFiles(const std::string& directory, const std::string& extension = "") const = 0;
    virtual bool CreateDirectory(const std::string& path) const = 0;
    virtual bool DeleteFile(const std::string& filename) const = 0;
    virtual bool FileExists(const std::string& filename) const = 0;
    virtual size_t GetFileSize(const std::string& filename) const = 0;
    
    // 경로 처리
    virtual std::string GetModDirectory() const = 0;
    virtual std::string GetGameDirectory() const = 0;
    virtual std::string GetTempDirectory() const = 0;
    virtual std::string JoinPath(const std::string& path1, const std::string& path2) const = 0;
};

// 메인 모드 API 인터페이스
class IModAPI {
public:
    virtual ~IModAPI() = default;
    
    // 하위 시스템 접근
    virtual ILogger* GetLogger() = 0;
    virtual IConfigManager* GetConfig() = 0;
    virtual IInputManager* GetInput() = 0;
    virtual IMemoryManager* GetMemory() = 0;
    virtual IHookManager* GetHooks() = 0;
    virtual IRenderManager* GetRenderer() = 0;
    virtual IFileSystem* GetFileSystem() = 0;
    
    // 이벤트 시스템
    virtual void RegisterEventHandler(const std::string& eventType, std::function<void(const Event&)> handler) = 0;
    virtual void UnregisterEventHandler(const std::string& eventType) = 0;
    virtual void FireEvent(const Event& event) = 0;
    virtual void FireEvent(const std::string& eventType, const std::string& sender = "") = 0;
    
    // 모드 간 통신
    virtual void RegisterModInterface(const std::string& name, void* interface) = 0;
    virtual void* GetModInterface(const std::string& name) = 0;
    virtual std::vector<std::string> GetAvailableInterfaces() const = 0;
    
    // 유틸리티
    virtual std::string GetModName() const = 0;
    virtual std::string GetAPIVersion() const = 0;
    virtual float GetDeltaTime() const = 0;
    virtual double GetTime() const = 0;
    
    // 간편 메서드들
    virtual void Log(const std::string& message, LogLevel level = LogLevel::Info) = 0;
    virtual void LogError(const std::string& message) = 0;
    virtual void LogWarning(const std::string& message) = 0;
    virtual void LogDebug(const std::string& message) = 0;
};

// 모드 인터페이스 (업데이트된 버전)
class IGameMod {
public:
    virtual ~IGameMod() = default;
    virtual bool Initialize(IModAPI* api) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetName() const = 0;
    virtual const char* GetVersion() const = 0;
    virtual const char* GetAuthor() const = 0;
    virtual const char* GetDescription() const = 0;
    virtual const char* GetAPIVersion() const = 0;
};

// 로거 구현
class Logger : public ILogger {
private:
    LogLevel currentLevel;
    bool fileLoggingEnabled;
    std::string logFilename;
    mutable std::mutex logMutex;
    std::ofstream logFile;
    
public:
    Logger() : currentLevel(LogLevel::Info), fileLoggingEnabled(false) {}
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void Log(LogLevel level, const std::string& message, const std::string& category = "") override {
        if (level < currentLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::string levelStr = GetLevelString(level);
        std::string categoryStr = category.empty() ? "" : "[" + category + "] ";
        
        std::string logMessage = "[" + std::string(std::ctime(&time_t)).substr(0, 24) + "] " +
                               "[" + levelStr + "] " + categoryStr + message;
        
        // 콘솔 출력
        if (level >= LogLevel::Error) {
            std::wcerr << StringToWString(logMessage) << std::endl;
        } else {
            std::wcout << StringToWString(logMessage) << std::endl;
        }
        
        // 파일 출력
        if (fileLoggingEnabled && logFile.is_open()) {
            logFile << logMessage << std::endl;
            logFile.flush();
        }
    }
    
    void SetLogLevel(LogLevel level) override {
        currentLevel = level;
    }
    
    void EnableFileLogging(const std::string& filename) override {
        std::lock_guard<std::mutex> lock(logMutex);
        
        if (logFile.is_open()) {
            logFile.close();
        }
        
        logFilename = filename;
        logFile.open(filename, std::ios::app);
        fileLoggingEnabled = logFile.is_open();
    }
    
    void DisableFileLogging() override {
        std::lock_guard<std::mutex> lock(logMutex);
        
        if (logFile.is_open()) {
            logFile.close();
        }
        fileLoggingEnabled = false;
    }
    
private:
    std::string GetLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }
};

// 설정 관리자 구현
class ConfigManager : public IConfigManager {
private:
    std::map<std::string, std::map<std::string, std::string>> data;
    std::string currentSection;
    mutable std::mutex configMutex;
    
public:
    ConfigManager() : currentSection("General") {}
    
    bool GetBool(const std::string& key, bool defaultValue = false) const override {
        std::lock_guard<std::mutex> lock(configMutex);
        std::string value = GetRawValue(key);
        if (value.empty()) return defaultValue;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return value == "true" || value == "1" || value == "yes";
    }
    
    int GetInt(const std::string& key, int defaultValue = 0) const override {
        std::lock_guard<std::mutex> lock(configMutex);
        std::string value = GetRawValue(key);
        if (value.empty()) return defaultValue;
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const override {
        std::lock_guard<std::mutex> lock(configMutex);
        std::string value = GetRawValue(key);
        if (value.empty()) return defaultValue;
        try {
            return std::stof(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const override {
        std::lock_guard<std::mutex> lock(configMutex);
        std::string value = GetRawValue(key);
        return value.empty() ? defaultValue : value;
    }
    
    void SetBool(const std::string& key, bool value) override {
        SetRawValue(key, value ? "true" : "false");
    }
    
    void SetInt(const std::string& key, int value) override {
        SetRawValue(key, std::to_string(value));
    }
    
    void SetFloat(const std::string& key, float value) override {
        SetRawValue(key, std::to_string(value));
    }
    
    void SetString(const std::string& key, const std::string& value) override {
        SetRawValue(key, value);
    }
    
    void SetSection(const std::string& section) override {
        std::lock_guard<std::mutex> lock(configMutex);
        currentSection = section;
    }
    
    std::vector<std::string> GetSections() const override {
        std::lock_guard<std::mutex> lock(configMutex);
        std::vector<std::string> sections;
        for (const auto& pair : data) {
            sections.push_back(pair.first);
        }
        return sections;
    }
    
    std::vector<std::string> GetKeys(const std::string& section = "") const override {
        std::lock_guard<std::mutex> lock(configMutex);
        std::string targetSection = section.empty() ? currentSection : section;
        std::vector<std::string> keys;
        
        auto it = data.find(targetSection);
        if (it != data.end()) {
            for (const auto& pair : it->second) {
                keys.push_back(pair.first);
            }
        }
        return keys;
    }
    
    bool LoadFromFile(const std::string& filename) override {
        std::lock_guard<std::mutex> lock(configMutex);
        
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        
        data.clear();
        std::string section = "General";
        std::string line;
        
        while (std::getline(file, line)) {
            // 공백 제거
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }
            
            if (line[0] == '[' && line.back() == ']') {
                section = line.substr(1, line.length() - 2);
                continue;
            }
            
            auto equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);
                
                // 키와 값의 공백 제거
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                
                data[section][key] = value;
            }
        }
        
        file.close();
        return true;
    }
    
    bool SaveToFile(const std::string& filename) const override {
        std::lock_guard<std::mutex> lock(configMutex);
        
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        
        for (const auto& sectionPair : data) {
            file << "[" << sectionPair.first << "]\n";
            for (const auto& keyPair : sectionPair.second) {
                file << keyPair.first << "=" << keyPair.second << "\n";
            }
            file << "\n";
        }
        
        file.close();
        return true;
    }
    
    bool HasKey(const std::string& key) const override {
        std::lock_guard<std::mutex> lock(configMutex);
        auto sectionIt = data.find(currentSection);
        if (sectionIt != data.end()) {
            return sectionIt->second.find(key) != sectionIt->second.end();
        }
        return false;
    }
    
    void RemoveKey(const std::string& key) override {
        std::lock_guard<std::mutex> lock(configMutex);
        auto sectionIt = data.find(currentSection);
        if (sectionIt != data.end()) {
            sectionIt->second.erase(key);
        }
    }
    
private:
    std::string GetRawValue(const std::string& key) const {
        auto sectionIt = data.find(currentSection);
        if (sectionIt != data.end()) {
            auto keyIt = sectionIt->second.find(key);
            if (keyIt != sectionIt->second.end()) {
                return keyIt->second;
            }
        }
        return "";
    }
    
    void SetRawValue(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(configMutex);
        data[currentSection][key] = value;
    }
};

// 메인 모드 API 구현
class ModAPI : public IModAPI {
private:
    std::unique_ptr<Logger> logger;
    std::unique_ptr<ConfigManager> configManager;
    std::string modName;
    std::string apiVersion;
    
    // 이벤트 시스템
    std::map<std::string, std::vector<std::function<void(const Event&)>>> eventHandlers;
    std::mutex eventMutex;
    
    // 모드 간 인터페이스
    std::map<std::string, void*> modInterfaces;
    std::mutex interfaceMutex;
    
    // 시간 관리
    std::chrono::high_resolution_clock::time_point startTime;
    float deltaTime;
    
public:
    ModAPI(const std::string& name) : modName(name), apiVersion("1.0.0"), deltaTime(0.0f) {
        logger = std::make_unique<Logger>();
        configManager = std::make_unique<ConfigManager>();
        startTime = std::chrono::high_resolution_clock::now();
        
        // 로그 파일 설정
        logger->EnableFileLogging("mod_" + name + ".log");
    }
    
    // IModAPI 구현
    ILogger* GetLogger() override { return logger.get(); }
    IConfigManager* GetConfig() override { return configManager.get(); }
    IInputManager* GetInput() override { return nullptr; }  // 추후 구현
    IMemoryManager* GetMemory() override { return nullptr; }  // 추후 구현
    IHookManager* GetHooks() override { return nullptr; }  // 추후 구현
    IRenderManager* GetRenderer() override { return nullptr; }  // 추후 구현
    IFileSystem* GetFileSystem() override { return nullptr; }  // 추후 구현
    
    void RegisterEventHandler(const std::string& eventType, std::function<void(const Event&)> handler) override {
        std::lock_guard<std::mutex> lock(eventMutex);
        eventHandlers[eventType].push_back(handler);
    }
    
    void UnregisterEventHandler(const std::string& eventType) override {
        std::lock_guard<std::mutex> lock(eventMutex);
        eventHandlers.erase(eventType);
    }
    
    void FireEvent(const Event& event) override {
        std::lock_guard<std::mutex> lock(eventMutex);
        auto it = eventHandlers.find(event.type);
        if (it != eventHandlers.end()) {
            for (auto& handler : it->second) {
                try {
                    handler(event);
                } catch (const std::exception& e) {
                    LogError("Event handler error: " + std::string(e.what()));
                }
            }
        }
    }
    
    void FireEvent(const std::string& eventType, const std::string& sender = "") override {
        Event event(eventType, sender.empty() ? modName : sender);
        FireEvent(event);
    }
    
    void RegisterModInterface(const std::string& name, void* interface) override {
        std::lock_guard<std::mutex> lock(interfaceMutex);
        modInterfaces[name] = interface;
    }
    
    void* GetModInterface(const std::string& name) override {
        std::lock_guard<std::mutex> lock(interfaceMutex);
        auto it = modInterfaces.find(name);
        return (it != modInterfaces.end()) ? it->second : nullptr;
    }
    
    std::vector<std::string> GetAvailableInterfaces() const override {
        std::lock_guard<std::mutex> lock(interfaceMutex);
        std::vector<std::string> interfaces;
        for (const auto& pair : modInterfaces) {
            interfaces.push_back(pair.first);
        }
        return interfaces;
    }
    
    std::string GetModName() const override { return modName; }
    std::string GetAPIVersion() const override { return apiVersion; }
    float GetDeltaTime() const override { return deltaTime; }
    
    double GetTime() const override {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        return duration.count() / 1000.0;
    }
    
    void Log(const std::string& message, LogLevel level = LogLevel::Info) override {
        logger->Log(level, message, modName);
    }
    
    void LogError(const std::string& message) override {
        logger->Log(LogLevel::Error, message, modName);
    }
    
    void LogWarning(const std::string& message) override {
        logger->Log(LogLevel::Warning, message, modName);
    }
    
    void LogDebug(const std::string& message) override {
        logger->Log(LogLevel::Debug, message, modName);
    }
    
    // 유틸리티 메서드
    void UpdateDeltaTime(float dt) {
        deltaTime = dt;
    }
};

// 예제 모드 구현
class ExampleAPIMod : public IGameMod {
private:
    IModAPI* api;
    bool enabled;
    float updateTimer;
    int updateCount;
    
public:
    ExampleAPIMod() : api(nullptr), enabled(true), updateTimer(0.0f), updateCount(0) {}
    
    bool Initialize(IModAPI* modAPI) override {
        api = modAPI;
        
        api->Log("ExampleAPIMod initializing...");
        
        // 설정 로드
        auto* config = api->GetConfig();
        config->SetSection("ExampleAPIMod");
        enabled = config->GetBool("enabled", true);
        
        // 이벤트 핸들러 등록
        api->RegisterEventHandler("game_start", [this](const Event& e) {
            api->Log("Game started event received!");
        });
        
        api->RegisterEventHandler("player_spawn", [this](const Event& e) {
            if (e.HasData("position")) {
                auto pos = e.GetData<std::string>("position");
                api->Log("Player spawned at: " + pos);
            }
        });
        
        // 커스텀 인터페이스 등록
        api->RegisterModInterface("ExampleInterface", this);
        
        api->Log("ExampleAPIMod initialized successfully");
        return true;
    }
    
    void Update(float deltaTime) override {
        if (!enabled) return;
        
        updateTimer += deltaTime;
        updateCount++;
        
        // 5초마다 상태 출력
        if (updateTimer >= 5.0f) {
            api->LogDebug("Update count: " + std::to_string(updateCount) + 
                         ", Total time: " + std::to_string(api->GetTime()));
            updateTimer = 0.0f;
            
            // 테스트 이벤트 발생
            Event testEvent("mod_update");
            testEvent.SetData<int>("update_count", updateCount);
            testEvent.SetData<float>("total_time", static_cast<float>(api->GetTime()));
            api->FireEvent(testEvent);
        }
    }
    
    void Render() override {
        // 렌더링 구현
    }
    
    void Shutdown() override {
        if (api) {
            // 설정 저장
            auto* config = api->GetConfig();
            config->SetSection("ExampleAPIMod");
            config->SetBool("enabled", enabled);
            config->SaveToFile("ExampleAPIMod.ini");
            
            api->Log("ExampleAPIMod shutdown");
        }
    }
    
    const char* GetName() const override { return "ExampleAPIMod"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetAuthor() const override { return "ModAPI Example"; }
    const char* GetDescription() const override { return "Example mod showcasing ModAPI features"; }
    const char* GetAPIVersion() const override { return "1.0.0"; }
    
    // 커스텀 인터페이스 메서드
    void ToggleEnabled() {
        enabled = !enabled;
        if (api) {
            api->Log("ExampleAPIMod " + std::string(enabled ? "enabled" : "disabled"));
        }
    }
    
    int GetUpdateCount() const {
        return updateCount;
    }
};

// API 테스트 프로그램
class APITestProgram {
private:
    std::unique_ptr<ModAPI> api;
    std::unique_ptr<ExampleAPIMod> testMod;
    bool running;
    
public:
    APITestProgram() : running(false) {}
    
    void Run() {
        std::wcout << L"=== Mod API Test Program ===" << std::endl;
        
        // API 초기화
        api = std::make_unique<ModAPI>(WStringToString(L"TestProgram"));
        testMod = std::make_unique<ExampleAPIMod>();
        
        // 모드 초기화
        if (!testMod->Initialize(api.get())) {
            std::wcerr << L"Failed to initialize test mod!" << std::endl;
            return;
        }
        
        running = true;
        
        std::wcout << L"Type 'help' for commands, 'quit' to exit" << std::endl;
        
        // 메인 루프
        auto lastTime = std::chrono::high_resolution_clock::now();
        std::thread inputThread(&APITestProgram::InputThread, this);
        
        while (running) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            // API 업데이트
            api->UpdateDeltaTime(deltaTime);
            
            // 모드 업데이트
            testMod->Update(deltaTime);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        if (inputThread.joinable()) {
            inputThread.join();
        }
        
        // 정리
        testMod->Shutdown();
        std::wcout << L"Test program finished" << std::endl;
    }
    
private:
    void InputThread() {
        std::wstring inputW;
        while (running) {
            std::wcout << L"\napi_test> ";
            std::getline(std::wcin, inputW);
            std::string input(inputW.begin(), inputW.end());
            
            if (input == "quit" || input == "exit") {
                running = false;
            } else if (input == "help") {
                ShowHelp();
            } else if (input == "status") {
                ShowStatus();
            } else if (input == "toggle") {
                testMod->ToggleEnabled();
            } else if (input == "event") {
                TestEvents();
            } else if (input == "config") {
                TestConfig();
            } else if (input == "interfaces") {
                ShowInterfaces();
            } else if (!input.empty()) {
                std::wcout << L"Unknown command: " << StringToWString(input) << std::endl;
            }
        }
    }
    
    void ShowHelp() {
        std::wcout << L"\nAvailable commands:" << std::endl;
        std::wcout << L"  help        - Show this help" << std::endl;
        std::wcout << L"  status      - Show mod status" << std::endl;
        std::wcout << L"  toggle      - Toggle test mod" << std::endl;
        std::wcout << L"  event       - Fire test events" << std::endl;
        std::wcout << L"  config      - Test configuration system" << std::endl;
        std::wcout << L"  interfaces  - Show available interfaces" << std::endl;
        std::wcout << L"  quit/exit   - Exit program" << std::endl;
    }
    
    void ShowStatus() {
        std::wcout << L"\n=== Mod Status ===" << std::endl;
        std::wcout << L"Mod: " << StringToWString(testMod->GetName()) << L" v" << StringToWString(testMod->GetVersion()) << std::endl;
        std::wcout << L"Author: " << StringToWString(testMod->GetAuthor()) << std::endl;
        std::wcout << L"Update Count: " << testMod->GetUpdateCount() << std::endl;
        std::wcout << L"API Version: " << StringToWString(api->GetAPIVersion()) << std::endl;
        std::wcout << L"Runtime: " << api->GetTime() << L" seconds" << std::endl;
    }
    
    void TestEvents() {
        std::wcout << L"Firing test events..." << std::endl;
        
        // 게임 시작 이벤트
        api->FireEvent("game_start");
        
        // 플레이어 스폰 이벤트
        Event spawnEvent("player_spawn");
        spawnEvent.SetData<std::string>("position", "100, 200, 300");
        spawnEvent.SetData<int>("health", 100);
        api->FireEvent(spawnEvent);
        
        std::wcout << L"Events fired" << std::endl;
    }
    
    void TestConfig() {
        std::wcout << L"Testing configuration system..." << std::endl;
        
        auto* config = api->GetConfig();
        config->SetSection("TestSection");
        
        config->SetString("test_string", "Hello World");
        config->SetInt("test_int", 42);
        config->SetFloat("test_float", 3.14f);
        config->SetBool("test_bool", true);
        
        std::wcout << L"String: " << StringToWString(config->GetString("test_string")) << std::endl;
        std::wcout << L"Int: " << config->GetInt("test_int") << std::endl;
        std::wcout << L"Float: " << config->GetFloat("test_float") << std::endl;
        std::wcout << L"Bool: " << (config->GetBool("test_bool") ? L"true" : L"false") << std::endl;
        
        config->SaveToFile("test_config.ini");
        std::wcout << L"Configuration saved to test_config.ini" << std::endl;
    }
    
    void ShowInterfaces() {
        std::wcout << L"\nAvailable mod interfaces:" << std::endl;
        auto interfaces = api->GetAvailableInterfaces();
        for (const auto& name : interfaces) {
            std::wcout << L"  - " << StringToWString(name) << std::endl;
        }
        
        if (interfaces.empty()) {
            std::wcout << L"  No interfaces registered" << std::endl;
        }
    }
};

// 모드 익스포트 매크로 사용 예제
EXPORT_MOD(ExampleAPIMod)

// 메인 함수
int main() {
    try {
        APITestProgram program;
        program.Run();
    } catch (const std::exception& e) {
        std::wcerr << L"Fatal error: " << StringToWString(e.what()) << std::endl;
        return 1;
    }
    
    return 0;
}