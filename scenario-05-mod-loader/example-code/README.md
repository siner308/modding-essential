# Mod Loader Example Code

게임에 모드를 동적으로 로드하고 관리하는 완전한 모드 로더 시스템입니다.

## 📁 파일 구조

```
example-code/
├── ModLoader.h            # 모드 로더 시스템 헤더
├── ModLoader.cpp          # 모드 로더 구현
├── ExampleMod.cpp         # 예제 모드 (DLL)
├── main.cpp               # 메인 애플리케이션
├── CMakeLists.txt         # CMake 빌드 스크립트
└── README.md              # 이 파일
```

## 🚀 빌드 방법

### 필수 요구사항

1. **Visual Studio 2019 이상** - MSVC 컴파일러
2. **Windows SDK** - Windows API 함수들
3. **CMake 3.16 이상** - 빌드 시스템

### Windows (Visual Studio)

```bash
# 프로젝트 빌드
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release

# 또는 제공된 배치 파일 사용
build.bat
```

### 빌드 결과

빌드 후 다음과 같은 구조가 생성됩니다:
```
build/
├── bin/Release/
│   └── ModLoader.exe      # 메인 애플리케이션
├── mods/Release/
│   └── ExampleMod.dll     # 예제 모드
└── config/
    └── ExampleMod.ini     # 모드 설정 파일
```

## 💻 사용법

### 기본 사용

```bash
# 빌드 디렉토리에서 실행
cd build
bin/Release/ModLoader.exe
```

### 모드 로더 초기화

```cpp
#include "ModLoader.h"

int main() {
    // 1. 모드 로더 생성
    ModLoader loader("./mods", "./config");
    
    // 2. 초기화
    if (!loader.Initialize()) {
        std::cout << "초기화 실패" << std::endl;
        return -1;
    }
    
    // 3. 모드 스캔 및 로드
    loader.ScanForMods();
    
    // 4. 메인 루프
    while (running) {
        loader.CheckForModUpdates();  // 핫 리로드
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 5. 정리
    loader.Shutdown();
    return 0;
}
```

### 모드 관리

```cpp
// 특정 모드 로드
loader.LoadMod("./mods/MyMod.dll");

// 모드 활성화/비활성화
loader.EnableMod("MyMod");
loader.DisableMod("MyMod");

// 모드 리로드 (개발 중 유용)
loader.ReloadMod("MyMod");

// 모드 언로드
loader.UnloadMod("MyMod");

// 로드된 모드 목록
auto mods = loader.GetLoadedMods();
for (const auto& mod : mods) {
    std::cout << mod.name << " v" << mod.version << std::endl;
}
```

### 모드 설정

```cpp
auto* config = loader.GetConfigManager();

// 설정 값 읽기/쓰기
config->SetString("MyMod", "player_name", "John");
config->SetInt("MyMod", "max_health", 100);
config->SetBool("MyMod", "god_mode", false);

std::string name = config->GetString("MyMod", "player_name", "DefaultName");
int health = config->GetInt("MyMod", "max_health", 50);
bool godMode = config->GetBool("MyMod", "god_mode", false);

// 설정 저장/로드
config->SaveConfig("MyMod");
config->LoadConfig("MyMod");
```

## 🛠️ 모드 개발

### 기본 모드 구조

```cpp
#include "ModLoader.h"

// 모드 정보 정의
IMPLEMENT_MOD("MyMod", "1.0.0", "작성자명", "모드 설명");

// 필수 함수들
MOD_EXPORT bool ModInit(ModLoader* loader) {
    MOD_LOG("MyMod 초기화 중...");
    
    // 설정 로드
    bool enabled = GET_CONFIG_BOOL("enabled", true);
    if (!enabled) {
        MOD_LOG("모드가 비활성화되어 있습니다");
        return false;
    }
    
    // 후킹 설치
    INSTALL_HOOK("MyHook", targetFunction, hookFunction, originalFunction);
    
    // 이벤트 핸들러 등록
    ModAPI::RegisterEventHandler("game_start", OnGameStart);
    
    MOD_LOG("MyMod 초기화 완료");
    return true;
}

MOD_EXPORT void ModCleanup() {
    MOD_LOG("MyMod 정리 중...");
    
    // 설정 저장
    SET_CONFIG_BOOL("enabled", true);
    
    MOD_LOG("MyMod 정리 완료");
}
```

### 함수 후킹

```cpp
// 원본 함수 포인터
typedef int(*OriginalFunction_t)(int param);
static OriginalFunction_t oOriginalFunction = nullptr;

// 후킹 함수
int HookFunction(int param) {
    MOD_LOG("함수가 호출됨: " + std::to_string(param));
    
    // 원본 함수 호출
    int result = oOriginalFunction(param);
    
    // 결과 수정
    return result * 2;
}

// 후킹 설치
bool InstallHook() {
    void* targetAddr = ModAPI::FindPattern("48 89 5C 24 08", "xxxxx");
    if (targetAddr) {
        return INSTALL_HOOK("MyHook", targetAddr, HookFunction, oOriginalFunction);
    }
    return false;
}
```

### 메모리 조작

```cpp
// 메모리 읽기/쓰기
void* playerHealthAddr = ModAPI::FindPattern("F3 0F 11 40 ?", "xxxx?");
if (playerHealthAddr) {
    float currentHealth;
    if (ModAPI::ReadMemory(playerHealthAddr, &currentHealth, sizeof(float))) {
        MOD_LOG("현재 체력: " + std::to_string(currentHealth));
        
        // 체력 증가
        float newHealth = currentHealth + 50.0f;
        ModAPI::WriteMemory(playerHealthAddr, &newHealth, sizeof(float));
    }
}
```

### 이벤트 시스템

```cpp
// 이벤트 핸들러
void OnPlayerLevelUp(const std::string& eventName, void* data) {
    int* newLevel = static_cast<int*>(data);
    MOD_LOG("플레이어 레벨업: " + std::to_string(*newLevel));
    
    // 레벨업 보너스 지급
    GivePlayerBonus(*newLevel);
}

// 이벤트 등록
ModAPI::RegisterEventHandler("player_levelup", OnPlayerLevelUp);

// 이벤트 발생
int playerLevel = 25;
ModAPI::TriggerEvent("player_levelup", &playerLevel);
```

### 모드 간 통신

```cpp
// 다른 모드에 메시지 전송
struct ModMessage {
    std::string sender = "MyMod";
    std::string type = "data_request";
    void* data = nullptr;
};

ModMessage msg;
msg.data = &someData;
ModAPI::TriggerEvent("mod_communication", &msg);

// 메시지 수신
void OnModMessage(const std::string& eventName, void* data) {
    ModMessage* msg = static_cast<ModMessage*>(data);
    if (msg->type == "data_request") {
        // 요청 처리
        ProcessDataRequest(msg);
    }
}
```

## 🎮 지원 기능

### 1. 동적 모드 로딩

- **DLL 로딩**: 런타임에 모드 DLL 로드/언로드
- **의존성 해결**: 모드 간 의존성 자동 처리
- **충돌 감지**: 호환되지 않는 모드 자동 차단
- **핫 리로드**: 개발 중 실시간 모드 재로딩

### 2. 설정 관리

- **INI 파일**: 모드별 설정 파일 자동 관리
- **타입 안전**: 문자열, 정수, 실수, 불린 타입 지원
- **기본값**: 설정이 없을 때 기본값 사용
- **자동 저장/로드**: 모드 시작/종료 시 자동 처리

### 3. 후킹 시스템

- **함수 후킹**: 게임 함수 가로채기 및 수정
- **VTable 후킹**: 가상 함수 테이블 후킹
- **메모리 패칭**: 직접 메모리 수정
- **안전 관리**: 모드 언로드 시 자동 복원

### 4. 이벤트 시스템

- **모드 간 통신**: 이벤트 기반 메시지 전달
- **게임 이벤트**: 게임 상태 변화 알림
- **비동기 처리**: 지연된 이벤트 처리 지원
- **타입 안전**: 강타입 이벤트 데이터 전달

## ⚠️ 주의사항

### 시스템 요구사항

1. **Windows 10/11**: Windows API 의존성
2. **관리자 권한**: 메모리 접근 및 DLL 인젝션
3. **Visual C++ 재배포**: MSVC 런타임 필요
4. **충분한 메모리**: 다중 모드 로딩

### 개발 가이드라인

```cpp
// 안전한 모드 개발
bool SafeModInit(ModLoader* loader) {
    try {
        // 1. 초기화 검증
        if (!loader) {
            MOD_LOG_ERROR("ModLoader가 null입니다");
            return false;
        }
        
        // 2. 의존성 확인
        if (!loader->IsModLoaded("RequiredMod")) {
            MOD_LOG_ERROR("필수 모드가 로드되지 않음: RequiredMod");
            return false;
        }
        
        // 3. 안전한 후킹
        if (!InstallHooksCarefully()) {
            MOD_LOG_ERROR("후킹 설치 실패");
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        MOD_LOG_ERROR("초기화 예외: " + std::string(e.what()));
        return false;
    }
}
```

### 성능 최적화

```cpp
// 효율적인 메모리 스캔
void* FindPatternOptimized(const std::string& pattern) {
    // 1. 캐시된 주소 확인
    static std::map<std::string, void*> addressCache;
    auto it = addressCache.find(pattern);
    if (it != addressCache.end()) {
        return it->second;
    }
    
    // 2. 제한된 범위에서 스캔
    void* result = ModAPI::FindPattern(pattern, "xxxxx", GetModuleBase(), GetModuleSize());
    
    // 3. 결과 캐싱
    if (result) {
        addressCache[pattern] = result;
    }
    
    return result;
}
```

## 🔧 고급 기능

### 1. 플러그인 아키텍처

```cpp
// 인터페이스 정의
class IGamePlugin {
public:
    virtual ~IGamePlugin() = default;
    virtual bool Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
};

// 플러그인 구현
class MyPlugin : public IGamePlugin {
public:
    bool Initialize() override {
        MOD_LOG("플러그인 초기화");
        return true;
    }
    
    void Update(float deltaTime) override {
        // 매 프레임 업데이트
    }
    
    void Shutdown() override {
        MOD_LOG("플러그인 종료");
    }
};

// 플러그인 등록
MOD_EXPORT IGamePlugin* CreatePlugin() {
    return new MyPlugin();
}
```

### 2. 스크립팅 지원

```cpp
// Lua 스크립트 지원 (개념)
class LuaScriptManager {
public:
    bool LoadScript(const std::string& filename);
    void ExecuteFunction(const std::string& functionName);
    void SetGlobal(const std::string& name, int value);
    int GetGlobal(const std::string& name);
};

// 스크립트 모드
MOD_EXPORT bool ModInit(ModLoader* loader) {
    auto scriptManager = std::make_unique<LuaScriptManager>();
    scriptManager->LoadScript("./scripts/mymod.lua");
    scriptManager->ExecuteFunction("onModInit");
    return true;
}
```

### 3. GUI 통합

```cpp
// ImGui 통합 (개념)
void RenderModGUI() {
    if (ImGui::Begin("Mod Settings")) {
        static bool enabled = GET_CONFIG_BOOL("enabled", true);
        if (ImGui::Checkbox("Enable Mod", &enabled)) {
            SET_CONFIG_BOOL("enabled", enabled);
        }
        
        static float value = GET_CONFIG_FLOAT("some_value", 1.0f);
        if (ImGui::SliderFloat("Value", &value, 0.0f, 10.0f)) {
            SET_CONFIG_FLOAT("some_value", value);
        }
    }
    ImGui::End();
}
```

## 🔗 관련 자료

- [DLL 인젝션 가이드](../../getting-started/dll-injection-guide.md)
- [함수 후킹 기법](../../getting-started/function-hooking-guide.md)
- [Scenario 05: 모드 로더 시스템](../README.md)
- [모드 개발 베스트 프랙티스](../../best-practices/mod-development.md)

## 📝 모드 템플릿

### 기본 템플릿

```bash
# 새 모드 템플릿 생성
create_mod_template.bat
```

생성된 템플릿:
```cpp
// MyMod.cpp - Generated mod template
#include "../ModLoader.h"

MOD_EXPORT int GetModAPIVersion() {
    return MOD_API_VERSION;
}

MOD_EXPORT const char* GetModInfo() {
    return "MyMod|1.0.0|YourName|Mod description here";
}

MOD_EXPORT bool ModInit(ModLoader* loader) {
    MOD_LOG("Initializing MyMod");
    // Add your initialization code here
    return true;
}

MOD_EXPORT void ModCleanup() {
    MOD_LOG("Cleaning up MyMod");
    // Add your cleanup code here
}
```

---

**⚡ 빌드 후 `build/bin/Release/ModLoader.exe`를 build 디렉토리에서 실행하세요!**

**🔧 모드 DLL은 `build/mods/` 폴더에, 설정 파일은 `build/config/` 폴더에 배치하세요.**