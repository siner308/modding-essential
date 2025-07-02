/*
 * Exercise 1: 기본 DLL 로더
 * 
 * 문제: 지정된 폴더의 DLL 파일들을 스캔하고 로드하는 기본 로더를 작성하세요.
 * 
 * 학습 목표:
 * - DLL 동적 로딩 기초
 * - 파일 시스템 조작
 * - 모드 관리 아키텍처
 */

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <map>
#include <memory>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
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

// 모드 인터페이스 정의
class IGameMod {
public:
    virtual ~IGameMod() = default;
    virtual bool Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetName() const = 0;
    virtual const char* GetVersion() const = 0;
    virtual const char* GetAuthor() const = 0;
    virtual const char* GetDescription() const = 0;
};

// C 스타일 익스포트 함수 타입 정의
extern "C" {
    typedef IGameMod*(*CreateModFunc)();
    typedef void(*DestroyModFunc)(IGameMod*);
    typedef const char*(*GetModInfoFunc)();
}

// 매크로로 모드 익스포트 간소화
#define EXPORT_MOD(className) \
    extern "C" __declspec(dllexport) IGameMod* CreateMod() { \
        return new className(); \
    } \
    extern "C" __declspec(dllexport) void DestroyMod(IGameMod* mod) { \
        delete mod; \
    } \
    extern "C" __declspec(dllexport) const char* GetModInfo() { \
        return #className " - Exported game modification"; \
    }

// 로드된 모드 정보
struct LoadedMod {
    std::string filename;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    HMODULE handle;
    IGameMod* instance;
    CreateModFunc createFunc;
    DestroyModFunc destroyFunc;
    GetModInfoFunc infoFunc;
    bool isEnabled;
    bool hasError;
    std::string errorMessage;
    std::chrono::system_clock::time_point loadTime;
    
    LoadedMod() : handle(nullptr), instance(nullptr), createFunc(nullptr), 
                 destroyFunc(nullptr), infoFunc(nullptr), isEnabled(false), hasError(false) {}
};

// 모드 로딩 통계
struct LoadingStats {
    int totalScanned = 0;
    int successfullyLoaded = 0;
    int failedToLoad = 0;
    int duplicates = 0;
    std::chrono::milliseconds totalLoadTime{0};
    std::vector<std::string> errorMessages;
};

class BasicModLoader {
private:
    std::vector<std::unique_ptr<LoadedMod>> loadedMods;
    std::map<std::string, size_t> modNameIndex;  // 이름으로 빠른 검색
    std::string modsDirectory;
    LoadingStats stats;
    bool isInitialized;
    mutable std::mutex modsMutex;  // 스레드 안전성
    
    // 로깅 시스템
    std::ofstream logFile;
    bool enableLogging;

public:
    BasicModLoader() : isInitialized(false), enableLogging(true) {
        // 기본 모드 디렉토리 설정
        modsDirectory = "./mods";
        
        // 로그 파일 초기화
        if (enableLogging) {
            logFile.open("mod_loader.log", std::ios::app);
            if (logFile.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                logFile << "\n=== Mod Loader Session Started: " 
                       << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << " ===\n";
            }
        }
    }
    
    ~BasicModLoader() {
        Shutdown();
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    bool Initialize(const std::string& directory = "") {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        if (isInitialized) {
            Log("Mod loader already initialized");
            return true;
        }
        
        if (!directory.empty()) {
            modsDirectory = directory;
        }
        
        Log("Initializing mod loader with directory: " + modsDirectory);
        
        // 모드 디렉토리 생성 (존재하지 않는 경우)
        if (!fs::exists(modsDirectory)) {
            std::error_code ec;
            if (!fs::create_directories(modsDirectory, ec)) {
                LogError("Failed to create mods directory: " + ec.message());
                return false;
            }
            Log("Created mods directory: " + modsDirectory);
        }
        
        // 디렉토리 접근 권한 확인
        if (!fs::is_directory(modsDirectory)) {
            LogError("Mods path is not a directory: " + modsDirectory);
            return false;
        }
        
        isInitialized = true;
        Log("Mod loader initialized successfully");
        return true;
    }
    
    void Shutdown() {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        if (!isInitialized) {
            return;
        }
        
        Log("Shutting down mod loader...");
        
        // 모든 모드 언로드
        UnloadAllMods();
        
        // 통계 출력
        PrintStatistics();
        
        isInitialized = false;
        Log("Mod loader shutdown complete");
    }
    
    bool ScanAndLoadMods() {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        if (!isInitialized) {
            LogError("Mod loader not initialized");
            return false;
        }
        
        Log("Starting mod scan and load process...");
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // 기존 모드들 언로드
        UnloadAllMods();
        
        // 통계 초기화
        stats = LoadingStats();
        
        // DLL 파일 스캔
        std::vector<fs::path> dllFiles;
        try {
            for (const auto& entry : fs::directory_iterator(modsDirectory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".dll") {
                    dllFiles.push_back(entry.path());
                    stats.totalScanned++;
                }
            }
        } catch (const fs::filesystem_error& e) {
            LogError("Failed to scan directory: " + std::string(e.what()));
            return false;
        }
        
        Log("Found " + std::to_string(dllFiles.size()) + " DLL files to process");
        
        // 파일명 순으로 정렬 (일관된 로딩 순서)
        std::sort(dllFiles.begin(), dllFiles.end());
        
        // 각 DLL 파일 로드 시도
        for (const auto& dllPath : dllFiles) {
            LoadSingleMod(dllPath);
        }
        
        // 로딩 시간 계산
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.totalLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 모든 모드 초기화
        InitializeLoadedMods();
        
        Log("Mod loading complete. Loaded " + std::to_string(stats.successfullyLoaded) + 
            "/" + std::to_string(stats.totalScanned) + " mods");
        
        return stats.successfullyLoaded > 0;
    }
    
    void UpdateMods(float deltaTime) {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        for (auto& mod : loadedMods) {
            if (mod->isEnabled && mod->instance && !mod->hasError) {
                try {
                    mod->instance->Update(deltaTime);
                } catch (const std::exception& e) {
                    LogError("Mod update failed for '" + mod->name + "': " + e.what());
                    mod->hasError = true;
                    mod->errorMessage = e.what();
                    mod->isEnabled = false;  // 오류 발생 시 비활성화
                } catch (...) {
                    LogError("Unknown error during mod update for: " + mod->name);
                    mod->hasError = true;
                    mod->errorMessage = "Unknown exception";
                    mod->isEnabled = false;
                }
            }
        }
    }
    
    // 모드 관리 함수들
    bool EnableMod(const std::string& modName) {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        auto it = modNameIndex.find(modName);
        if (it != modNameIndex.end() && it->second < loadedMods.size()) {
            auto& mod = loadedMods[it->second];
            if (!mod->hasError && mod->instance) {
                mod->isEnabled = true;
                Log("Enabled mod: " + modName);
                return true;
            }
        }
        return false;
    }
    
    bool DisableMod(const std::string& modName) {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        auto it = modNameIndex.find(modName);
        if (it != modNameIndex.end() && it->second < loadedMods.size()) {
            auto& mod = loadedMods[it->second];
            mod->isEnabled = false;
            Log("Disabled mod: " + modName);
            return true;
        }
        return false;
    }
    
    bool ReloadMod(const std::string& modName) {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        auto it = modNameIndex.find(modName);
        if (it != modNameIndex.end() && it->second < loadedMods.size()) {
            auto& mod = loadedMods[it->second];
            
            // 기존 모드 종료 및 언로드
            if (mod->instance) {
                try {
                    mod->instance->Shutdown();
                } catch (...) {
                    LogError("Error during mod shutdown: " + modName);
                }
                
                if (mod->destroyFunc) {
                    mod->destroyFunc(mod->instance);
                }
                mod->instance = nullptr;
            }
            
            if (mod->handle) {
                FreeLibrary(mod->handle);
                mod->handle = nullptr;
            }
            
            // 다시 로드
            fs::path modPath = fs::path(modsDirectory) / mod->filename;
            if (LoadSingleMod(modPath)) {
                Log("Successfully reloaded mod: " + modName);
                return true;
            }
        }
        
        LogError("Failed to reload mod: " + modName);
        return false;
    }
    
    std::vector<std::string> GetLoadedModNames() const {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        std::vector<std::string> names;
        for (const auto& mod : loadedMods) {
            names.push_back(mod->name);
        }
        return names;
    }
    
    LoadedMod* GetModInfo(const std::string& modName) const {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        auto it = modNameIndex.find(modName);
        if (it != modNameIndex.end() && it->second < loadedMods.size()) {
            return loadedMods[it->second].get();
        }
        return nullptr;
    }
    
    void PrintModList() const {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        std::wcout << L"\n=== Loaded Mods ===" << std::endl;
        
        if (loadedMods.empty()) {
            std::wcout << L"No mods loaded." << std::endl;
            return;
        }
        
        for (size_t i = 0; i < loadedMods.size(); ++i) {
            const auto& mod = loadedMods[i];
            std::wcout << L"[" << (i + 1) << L"] " << StringToWString(mod->name) 
                     << L" v" << StringToWString(mod->version) 
                     << L" by " << StringToWString(mod->author) 
                     << (mod->isEnabled ? L" [ENABLED]" : L" [DISABLED]");
            
            if (mod->hasError) {
                std::wcout << L" [ERROR: " << StringToWString(mod->errorMessage) << L"]";
            }
            
            std::wcout << std::endl;
            std::wcout << L"    File: " << StringToWString(mod->filename) << std::endl;
            std::wcout << L"    Description: " << StringToWString(mod->description) << std::endl;
            
            // 로드 시간 표시
            auto time_t = std::chrono::system_clock::to_time_t(mod->loadTime);
            std::wcout << L"    Loaded: " << std::put_time(std::localtime(&time_t), L"%Y-%m-%d %H:%M:%S") << std::endl;
            std::wcout << std::endl;
        }
    }
    
    void PrintStatistics() const {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        std::wcout << L"\n=== Loading Statistics ===" << std::endl;
        std::wcout << L"Total files scanned: " << stats.totalScanned << std::endl;
        std::wcout << L"Successfully loaded: " << stats.successfullyLoaded << std::endl;
        std::wcout << L"Failed to load: " << stats.failedToLoad << std::endl;
        std::wcout << L"Duplicates ignored: " << stats.duplicates << std::endl;
        std::wcout << L"Total loading time: " << stats.totalLoadTime.count() << L"ms" << std::endl;
        
        if (!stats.errorMessages.empty()) {
            std::wcout << L"\nErrors encountered:" << std::endl;
            for (const auto& error : stats.errorMessages) {
                std::wcout << L"  - " << StringToWString(error) << std::endl;
            }
        }
        std::wcout << L"=======================" << std::endl;
    }
    
    // 설정 파일 지원
    bool SaveModConfiguration(const std::string& configFile = "mod_config.ini") const {
        std::lock_guard<std::mutex> lock(modsMutex);
        
        std::ofstream file(configFile);
        if (!file.is_open()) {
            LogError("Failed to create config file: " + configFile);
            return false;
        }
        
        file << "[ModLoader]\n";
        file << "mods_directory=" << modsDirectory << "\n";
        file << "total_mods=" << loadedMods.size() << "\n\n";
        
        for (const auto& mod : loadedMods) {
            file << "[" << mod->name << "]\n";
            file << "filename=" << mod->filename << "\n";
            file << "version=" << mod->version << "\n";
            file << "author=" << mod->author << "\n";
            file << "enabled=" << (mod->isEnabled ? "true" : "false") << "\n";
            file << "has_error=" << (mod->hasError ? "true" : "false") << "\n";
            if (mod->hasError) {
                file << "error_message=" << mod->errorMessage << "\n";
            }
            file << "\n";
        }
        
        file.close();
        Log("Saved mod configuration to: " + configFile);
        return true;
    }
    
    bool LoadModConfiguration(const std::string& configFile = "mod_config.ini") {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            Log("Config file not found: " + configFile);
            return false;
        }
        
        std::string line;
        std::string currentSection;
        
        while (std::getline(file, line)) {
            // 공백 제거
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;  // 빈 줄이나 주석 건너뛰기
            }
            
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }
            
            auto equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);
                
                if (currentSection == "ModLoader" && key == "mods_directory") {
                    modsDirectory = value;
                }
                // 개별 모드 설정은 로드 후 적용
            }
        }
        
        file.close();
        Log("Loaded mod configuration from: " + configFile);
        return true;
    }

private:
    bool LoadSingleMod(const fs::path& dllPath) {
        auto mod = std::make_unique<LoadedMod>();
        mod->filename = dllPath.filename().string();
        mod->loadTime = std::chrono::system_clock::now();
        
        Log("Attempting to load: " + mod->filename);
        
        // DLL 유효성 검사
        if (!ValidateDLL(dllPath)) {
            LogError("DLL validation failed: " + mod->filename);
            stats.failedToLoad++;
            return false;
        }
        
        // DLL 로드
        mod->handle = LoadLibraryA(dllPath.string().c_str());
        if (!mod->handle) {
            DWORD error = GetLastError();
            std::string errorMsg = "Failed to load DLL (Error " + std::to_string(error) + "): " + mod->filename;
            LogError(errorMsg);
            stats.errorMessages.push_back(errorMsg);
            stats.failedToLoad++;
            return false;
        }
        
        // 필수 함수들 가져오기
        mod->createFunc = reinterpret_cast<CreateModFunc>(GetProcAddress(mod->handle, "CreateMod"));
        mod->destroyFunc = reinterpret_cast<DestroyModFunc>(GetProcAddress(mod->handle, "DestroyMod"));
        mod->infoFunc = reinterpret_cast<GetModInfoFunc>(GetProcAddress(mod->handle, "GetModInfo"));
        
        if (!mod->createFunc || !mod->destroyFunc) {
            LogError("Required functions not found in: " + mod->filename);
            FreeLibrary(mod->handle);
            stats.failedToLoad++;
            return false;
        }
        
        // 모드 인스턴스 생성
        try {
            mod->instance = mod->createFunc();
            if (!mod->instance) {
                LogError("Failed to create mod instance: " + mod->filename);
                FreeLibrary(mod->handle);
                stats.failedToLoad++;
                return false;
            }
        } catch (const std::exception& e) {
            LogError("Exception during mod creation: " + std::string(e.what()));
            FreeLibrary(mod->handle);
            stats.failedToLoad++;
            return false;
        }
        
        // 모드 정보 수집
        mod->name = mod->instance->GetName();
        mod->version = mod->instance->GetVersion();
        mod->author = mod->instance->GetAuthor();
        mod->description = mod->instance->GetDescription();
        
        // 중복 이름 확인
        if (modNameIndex.find(mod->name) != modNameIndex.end()) {
            LogError("Duplicate mod name detected: " + mod->name);
            mod->destroyFunc(mod->instance);
            FreeLibrary(mod->handle);
            stats.duplicates++;
            return false;
        }
        
        // 인덱스에 추가
        modNameIndex[mod->name] = loadedMods.size();
        
        mod->isEnabled = true;  // 기본적으로 활성화
        
        loadedMods.push_back(std::move(mod));
        stats.successfullyLoaded++;
        
        Log("Successfully loaded mod: " + loadedMods.back()->name + " v" + loadedMods.back()->version);
        return true;
    }
    
    bool ValidateDLL(const fs::path& dllPath) {
        // 파일 크기 검사
        std::error_code ec;
        auto fileSize = fs::file_size(dllPath, ec);
        if (ec) {
            LogError("Cannot get file size: " + dllPath.string());
            return false;
        }
        
        if (fileSize > 50 * 1024 * 1024) {  // 50MB 제한
            LogError("File too large: " + dllPath.string());
            return false;
        }
        
        if (fileSize < 1024) {  // 최소 크기 검사
            LogError("File too small to be valid DLL: " + dllPath.string());
            return false;
        }
        
        // PE 헤더 간단 검증
        std::ifstream file(dllPath, std::ios::binary);
        if (!file.is_open()) {
            LogError("Cannot open file for validation: " + dllPath.string());
            return false;
        }
        
        // DOS 헤더 확인
        char dosHeader[2];
        file.read(dosHeader, 2);
        if (dosHeader[0] != 'M' || dosHeader[1] != 'Z') {
            LogError("Invalid PE file (DOS header): " + dllPath.string());
            return false;
        }
        
        file.close();
        return true;
    }
    
    void InitializeLoadedMods() {
        for (auto& mod : loadedMods) {
            if (mod->instance && !mod->hasError) {
                try {
                    if (mod->instance->Initialize()) {
                        Log("Initialized mod: " + mod->name);
                    } else {
                        LogError("Mod initialization failed: " + mod->name);
                        mod->hasError = true;
                        mod->errorMessage = "Initialization failed";
                        mod->isEnabled = false;
                    }
                } catch (const std::exception& e) {
                    LogError("Exception during mod initialization: " + mod->name + " - " + e.what());
                    mod->hasError = true;
                    mod->errorMessage = e.what();
                    mod->isEnabled = false;
                } catch (...) {
                    LogError("Unknown exception during mod initialization: " + mod->name);
                    mod->hasError = true;
                    mod->errorMessage = "Unknown exception";
                    mod->isEnabled = false;
                }
            }
        }
    }
    
    void UnloadAllMods() {
        for (auto& mod : loadedMods) {
            if (mod->instance) {
                try {
                    mod->instance->Shutdown();
                } catch (...) {
                    LogError("Error during mod shutdown: " + mod->name);
                }
                
                if (mod->destroyFunc) {
                    mod->destroyFunc(mod->instance);
                }
                mod->instance = nullptr;
            }
            
            if (mod->handle) {
                FreeLibrary(mod->handle);
                mod->handle = nullptr;
            }
        }
        
        loadedMods.clear();
        modNameIndex.clear();
    }
    
    void Log(const std::string& message) const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        if (logFile.is_open()) {
            logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] " 
                   << message << std::endl;
            logFile.flush();
        }
        
        std::wcout << L"[" << std::put_time(std::localtime(&time_t), L"%H:%M:%S") << L"] " 
                 << StringToWString(message) << std::endl;
    }
    
    void LogError(const std::string& message) const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        if (logFile.is_open()) {
            logFile << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] ERROR: " 
                   << message << std::endl;
            logFile.flush();
        }
        
        std::wcerr << L"[" << std::put_time(std::localtime(&time_t), L"%H:%M:%S") << L"] ERROR: " 
                 << StringToWString(message) << std::endl;
    }
};

// 간단한 테스트 모드 구현 예제
class TestMod : public IGameMod {
public:
    bool Initialize() override {
        std::wcout << L"TestMod: Initialize called" << std::endl;
        return true;
    }
    
    void Update(float deltaTime) override {
        // 매 프레임 호출되므로 로그는 생략
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 300 == 0) {  // 5초마다 한 번씩 출력 (60 FPS 가정)
            std::wcout << L"TestMod: Update called (frame " << frameCount << L")" << std::endl;
        }
    }
    
    void Shutdown() override {
        std::wcout << L"TestMod: Shutdown called" << std::endl;
    }
    
    const char* GetName() const override { return "TestMod"; }
    const char* GetVersion() const override { return "1.0.0"; }
    const char* GetAuthor() const override { return "ModLoader Example"; }
    const char* GetDescription() const override { return "A simple test mod for demonstration"; }
};

// 콘솔 인터페이스
class ModLoaderConsole {
private:
    BasicModLoader loader;
    bool running;
    
public:
    ModLoaderConsole() : running(false) {}
    
    void Run() {
        std::wcout << L"=== Basic Mod Loader Console ===" << std::endl;
        std::wcout << L"Type 'help' for available commands" << std::endl;
        
        if (!loader.Initialize()) {
            std::wcerr << L"Failed to initialize mod loader!" << std::endl;
            return;
        }
        
        // 설정 파일 로드 시도
        loader.LoadModConfiguration();
        
        running = true;
        std::wstring inputW;
        
        while (running) {
            std::wcout << L"\nmod_loader> ";
            std::getline(std::wcin, inputW);
            std::string input(inputW.begin(), inputW.end());
            
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
        } else if (command == "scan" || command == "load") {
            loader.ScanAndLoadMods();
        } else if (command == "list") {
            loader.PrintModList();
        } else if (command == "stats") {
            loader.PrintStatistics();
        } else if (command == "enable") {
            std::string modName;
            iss >> modName;
            if (!modName.empty()) {
                if (loader.EnableMod(modName)) {
                    std::wcout << L"Enabled mod: " << StringToWString(modName) << std::endl;
                } else {
                    std::wcout << L"Failed to enable mod: " << StringToWString(modName) << std::endl;
                }
            } else {
                std::wcout << L"Usage: enable <mod_name>" << std::endl;
            }
        } else if (command == "disable") {
            std::string modName;
            iss >> modName;
            if (!modName.empty()) {
                if (loader.DisableMod(modName)) {
                    std::wcout << L"Disabled mod: " << StringToWString(modName) << std::endl;
                } else {
                    std::wcout << L"Failed to disable mod: " << StringToWString(modName) << std::endl;
                }
            } else {
                std::wcout << L"Usage: disable <mod_name>" << std::endl;
            }
        } else if (command == "reload") {
            std::string modName;
            iss >> modName;
            if (!modName.empty()) {
                if (loader.ReloadMod(modName)) {
                    std::wcout << L"Reloaded mod: " << StringToWString(modName) << std::endl;
                } else {
                    std::wcout << L"Failed to reload mod: " << StringToWString(modName) << std::endl;
                }
            } else {
                std::wcout << L"Usage: reload <mod_name>" << std::endl;
            }
        } else if (command == "info") {
            std::string modName;
            iss >> modName;
            if (!modName.empty()) {
                auto* modInfo = loader.GetModInfo(modName);
                if (modInfo) {
                    std::wcout << L"=== Mod Information ===" << std::endl;
                    std::wcout << L"Name: " << StringToWString(modInfo->name) << std::endl;
                    std::wcout << L"Version: " << StringToWString(modInfo->version) << std::endl;
                    std::wcout << L"Author: " << StringToWString(modInfo->author) << std::endl;
                    std::wcout << L"Description: " << StringToWString(modInfo->description) << std::endl;
                    std::wcout << L"File: " << StringToWString(modInfo->filename) << std::endl;
                    std::wcout << L"Enabled: " << (modInfo->isEnabled ? L"Yes" : L"No") << std::endl;
                    std::wcout << L"Has Error: " << (modInfo->hasError ? L"Yes" : L"No") << std::endl;
                    if (modInfo->hasError) {
                        std::wcout << L"Error: " << StringToWString(modInfo->errorMessage) << std::endl;
                    }
                } else {
                    std::wcout << L"Mod not found: " << StringToWString(modName) << std::endl;
                }
            } else {
                std::wcout << L"Usage: info <mod_name>" << std::endl;
            }
        } else if (command == "save") {
            if (loader.SaveModConfiguration()) {
                std::wcout << L"Configuration saved" << std::endl;
            } else {
                std::wcout << L"Failed to save configuration" << std::endl;
            }
        } else if (command == "update") {
            // 테스트용 업데이트 (1프레임)
            loader.UpdateMods(0.016f);  // 60 FPS 가정
            std::wcout << L"Updated all mods (1 frame)" << std::endl;
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
        std::wcout << L"  help          - Show this help message" << std::endl;
        std::wcout << L"  scan/load     - Scan and load mods from directory" << std::endl;
        std::wcout << L"  list          - List all loaded mods" << std::endl;
        std::wcout << L"  stats         - Show loading statistics" << std::endl;
        std::wcout << L"  enable <mod>  - Enable a specific mod" << std::endl;
        std::wcout << L"  disable <mod> - Disable a specific mod" << std::endl;
        std::wcout << L"  reload <mod>  - Reload a specific mod" << std::endl;
        std::wcout << L"  info <mod>    - Show detailed mod information" << std::endl;
        std::wcout << L"  save          - Save current configuration" << std::endl;
        std::wcout << L"  update        - Update all mods (test)" << std::endl;
        std::wcout << L"  quit/exit     - Exit the program" << std::endl;
    }
};

// 메인 함수 - 콘솔 애플리케이션으로 실행
int main() {
    try {
        ModLoaderConsole console;
        console.Run();
    } catch (const std::exception& e) {
        std::wcerr << L"Fatal error: " << StringToWString(e.what()) << std::endl;
        return 1;
    }
    
    return 0;
}