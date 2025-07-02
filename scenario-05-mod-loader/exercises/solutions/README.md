# Exercise Solutions - 모드 로더 시스템

이 폴더는 scenario-05-mod-loader의 연습문제 해답들을 포함합니다.

## 📋 연습문제 목록

### Exercise 1: 기본 DLL 로더
**문제**: 지정된 폴더의 DLL 파일들을 스캔하고 로드하는 기본 로더를 작성하세요.

**해답 파일**: `exercise1_basic_loader.cpp`

### Exercise 2: 모드 API 시스템
**문제**: 모드들이 사용할 수 있는 공통 API 인터페이스를 설계하고 구현하세요.

**해답 파일**: `exercise2_mod_api.cpp`

### Exercise 3: 설정 관리 시스템
**문제**: 모드별 설정을 INI 파일로 저장/로드하는 시스템을 만드세요.

**해답 파일**: `exercise3_config_system.cpp`

### Exercise 4: 의존성 해결
**문제**: 모드 간 의존성을 분석하고 올바른 순서로 로드하는 시스템을 구현하세요.

**해답 파일**: `exercise4_dependency_resolver.cpp`

### Exercise 5: 핫 리로드
**문제**: 개발 중 모드 파일이 변경되면 자동으로 재로드하는 기능을 만드세요.

**해답 파일**: `exercise5_hot_reload.cpp`

## 🔧 학습 목표

### 시스템 아키텍처
1. **플러그인 패턴**: 확장 가능한 시스템 설계
2. **의존성 주입**: 느슨한 결합 구조
3. **이벤트 시스템**: 모드 간 통신
4. **리소스 관리**: 메모리 및 파일 핸들 관리

### Windows API 활용
1. **DLL 관리**: `LoadLibrary`, `GetProcAddress`, `FreeLibrary`
2. **파일 시스템**: 파일 변경 감지, 디렉토리 스캔
3. **프로세스 관리**: 모듈 열거, 메모리 보호
4. **스레딩**: 비동기 작업 처리

## 🛠️ 핵심 구현

### 모드 인터페이스 정의
```cpp
// 표준 모드 인터페이스
class IGameMod {
public:
    virtual ~IGameMod() = default;
    virtual bool Initialize(IModAPI* api) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetName() const = 0;
    virtual const char* GetVersion() const = 0;
};

// C 스타일 익스포트 함수
extern "C" {
    typedef IGameMod*(*CreateModFunc)();
    typedef void(*DestroyModFunc)(IGameMod*);
}

#define EXPORT_MOD(className) \
    extern "C" __declspec(dllexport) IGameMod* CreateMod() { \
        return new className(); \
    } \
    extern "C" __declspec(dllexport) void DestroyMod(IGameMod* mod) { \
        delete mod; \
    }
```

### 의존성 해결 알고리즘
```cpp
class DependencyResolver {
private:
    struct ModNode {
        std::string name;
        std::vector<std::string> dependencies;
        std::vector<std::string> dependents;
        bool visited = false;
        bool inStack = false;
    };
    
public:
    std::vector<std::string> ResolveDependencies(const std::vector<ModInfo>& mods) {
        std::map<std::string, ModNode> nodes;
        
        // 그래프 구성
        for (const auto& mod : mods) {
            nodes[mod.name] = {mod.name, mod.dependencies, {}, false, false};
        }
        
        // 역방향 의존성 구성
        for (auto& [name, node] : nodes) {
            for (const auto& dep : node.dependencies) {
                if (nodes.find(dep) != nodes.end()) {
                    nodes[dep].dependents.push_back(name);
                }
            }
        }
        
        // 위상 정렬
        std::vector<std::string> result;
        for (auto& [name, node] : nodes) {
            if (!node.visited) {
                if (!TopologicalSort(nodes, name, result)) {
                    throw std::runtime_error("Circular dependency detected");
                }
            }
        }
        
        return result;
    }
    
private:
    bool TopologicalSort(std::map<std::string, ModNode>& nodes, 
                        const std::string& name, 
                        std::vector<std::string>& result) {
        auto& node = nodes[name];
        
        if (node.inStack) return false; // 순환 참조
        if (node.visited) return true;
        
        node.inStack = true;
        
        // 의존성 먼저 처리
        for (const auto& dep : node.dependencies) {
            if (nodes.find(dep) != nodes.end()) {
                if (!TopologicalSort(nodes, dep, result)) {
                    return false;
                }
            }
        }
        
        node.visited = true;
        node.inStack = false;
        result.push_back(name);
        
        return true;
    }
};
```

### 파일 변경 감지
```cpp
class FileWatcher {
private:
    HANDLE hDir;
    std::thread watchThread;
    std::atomic<bool> stopFlag{false};
    std::function<void(const std::string&)> callback;
    
public:
    FileWatcher(const std::string& directory, std::function<void(const std::string&)> cb) 
        : callback(cb) {
        
        hDir = CreateFileA(directory.c_str(), 
                          FILE_LIST_DIRECTORY,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          nullptr, OPEN_EXISTING, 
                          FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        
        if (hDir != INVALID_HANDLE_VALUE) {
            watchThread = std::thread(&FileWatcher::WatchThread, this);
        }
    }
    
    ~FileWatcher() {
        stopFlag = true;
        if (watchThread.joinable()) {
            watchThread.join();
        }
        if (hDir != INVALID_HANDLE_VALUE) {
            CloseHandle(hDir);
        }
    }
    
private:
    void WatchThread() {
        char buffer[4096];
        DWORD bytesReturned;
        
        while (!stopFlag) {
            if (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE,
                                    FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                                    &bytesReturned, nullptr, nullptr)) {
                
                auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
                do {
                    std::wstring filename(info->FileName, info->FileNameLength / sizeof(WCHAR));
                    std::string filenameStr(filename.begin(), filename.end());
                    
                    if (filenameStr.ends_with(".dll")) {
                        callback(filenameStr);
                    }
                    
                    if (info->NextEntryOffset == 0) break;
                    info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<char*>(info) + info->NextEntryOffset);
                } while (true);
            }
        }
    }
};
```

## 🔌 모드 개발 예제

### 기본 모드 구조
```cpp
// ExampleMod.cpp
#include "ModAPI.h"

class ExampleMod : public IGameMod {
private:
    IModAPI* api;
    bool enabled = true;
    
public:
    bool Initialize(IModAPI* modAPI) override {
        api = modAPI;
        
        // 설정 로드
        enabled = api->GetConfigBool("enabled", true);
        
        // 이벤트 등록
        api->RegisterEventHandler("game_start", [this](const Event& e) {
            OnGameStart();
        });
        
        // 후킹 설치
        api->InstallHook("GameFunction", reinterpret_cast<void*>(0x12345678), 
                        reinterpret_cast<void*>(HookedFunction), 
                        reinterpret_cast<void**>(&originalFunction));
        
        api->Log("ExampleMod initialized");
        return true;
    }
    
    void Update(float deltaTime) override {
        if (!enabled) return;
        
        // 매 프레임 업데이트 로직
        ProcessInput();
        UpdateVisuals();
    }
    
    void Shutdown() override {
        // 설정 저장
        api->SetConfigBool("enabled", enabled);
        api->SaveConfig();
        
        api->Log("ExampleMod shutdown");
    }
    
    const char* GetName() const override { return "ExampleMod"; }
    const char* GetVersion() const override { return "1.0.0"; }
    
private:
    void OnGameStart() {
        api->Log("Game started - ExampleMod active");
    }
    
    static void HookedFunction() {
        // 후킹된 함수 로직
        originalFunction();
    }
    
    static void(*originalFunction)();
    
    void ProcessInput() {
        if (api->IsKeyPressed(VK_F1)) {
            enabled = !enabled;
            api->Log(enabled ? "ExampleMod enabled" : "ExampleMod disabled");
        }
    }
    
    void UpdateVisuals() {
        // 시각적 효과 업데이트
    }
};

// 모드 익스포트
EXPORT_MOD(ExampleMod)
```

### 모드 설정 파일
```ini
# ExampleMod.ini
[General]
enabled=true
debug=false

[Graphics]
enable_effects=true
effect_intensity=0.8

[Controls]
toggle_key=F1
menu_key=F2

[Dependencies]
required_mods=CoreMod,UIFramework
optional_mods=AdvancedGraphics
conflicts=OldMod,IncompatibleMod
```

## 📊 성능 고려사항

### 효율적인 모드 관리
```cpp
class ModManager {
private:
    std::vector<std::unique_ptr<LoadedMod>> mods;
    std::unordered_map<std::string, size_t> modIndex;
    
public:
    void UpdateMods(float deltaTime) {
        // 활성화된 모드만 업데이트
        for (auto& mod : mods) {
            if (mod->IsEnabled() && mod->GetMod()) {
                try {
                    mod->GetMod()->Update(deltaTime);
                } catch (const std::exception& e) {
                    LogError("Mod update failed: " + std::string(e.what()));
                    mod->SetEnabled(false); // 문제 모드 비활성화
                }
            }
        }
    }
    
    IGameMod* FindMod(const std::string& name) {
        auto it = modIndex.find(name);
        if (it != modIndex.end() && it->second < mods.size()) {
            return mods[it->second]->GetMod();
        }
        return nullptr;
    }
};
```

### 메모리 관리
```cpp
class ModMemoryManager {
private:
    std::unordered_map<std::string, std::unique_ptr<MemoryPool>> modPools;
    
public:
    void* AllocateForMod(const std::string& modName, size_t size) {
        auto& pool = modPools[modName];
        if (!pool) {
            pool = std::make_unique<MemoryPool>(1024 * 1024); // 1MB per mod
        }
        return pool->Allocate(size);
    }
    
    void DeallocateForMod(const std::string& modName) {
        modPools.erase(modName);
    }
};
```

## ⚠️ 보안 고려사항

### 모드 검증
```cpp
bool ValidateMod(const std::filesystem::path& modPath) {
    // 1. 파일 크기 검증
    auto fileSize = std::filesystem::file_size(modPath);
    if (fileSize > 100 * 1024 * 1024) { // 100MB 제한
        return false;
    }
    
    // 2. PE 헤더 검증
    if (!IsValidPE(modPath)) {
        return false;
    }
    
    // 3. 디지털 서명 검증 (선택사항)
    if (!VerifyDigitalSignature(modPath)) {
        LogWarning("Mod is not digitally signed: " + modPath.string());
    }
    
    // 4. 악성 코드 검사 (기본적인 패턴 매칭)
    if (ContainsMaliciousPatterns(modPath)) {
        LogError("Mod contains suspicious patterns: " + modPath.string());
        return false;
    }
    
    return true;
}
```

---

**🔌 목표: 확장 가능하고 안정적인 모드 로더 시스템 구축 능력 습득**