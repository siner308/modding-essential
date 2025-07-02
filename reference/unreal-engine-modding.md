# 언리얼 엔진 게임 모딩 가이드

언리얼 엔진 4/5로 개발된 게임들의 모딩 기법과 도구들을 종합적으로 정리합니다.

## 📋 목차

1. [언리얼 엔진 모딩 개요](#언리얼-엔진-모딩-개요)
2. [UE4SS - 스크립팅 시스템](#ue4ss---스크립팅-시스템)
3. [UETools - 개발자 도구](#uetools---개발자-도구)
4. [UI 모딩 - ImGui 통합](#ui-모딩---imgui-통합)
5. [Blueprint 모딩](#blueprint-모딩)
6. [실전 예제: Lies of P](#실전-예제-lies-of-p)

---

## 🎮 언리얼 엔진 모딩 개요

### 언리얼 엔진의 특징
- **Blueprint 시스템**: 비주얼 스크립팅 언어
- **Reflection 시스템**: 런타임 타입 정보 제공
- **Object 시스템**: 모든 게임 객체의 기반
- **PAK 파일 시스템**: 게임 에셋 패키징

### 주요 모딩 접근법

#### 1. **PAK 기반 모딩** (가장 일반적이고 안전)
```
게임폴더/Content/Paks/~mods/
├── Z_ModLoader_P.pak    # 모드 로더 (우선순위 최고)
├── MyMod_P.pak          # 패키지된 모드 파일
├── MyTextureMod_P.pak   # 텍스처 교체 모드
└── MyUIMod_P.pak        # UI 수정 모드
```

**PAK 파일 명명 규칙**:
```bash
# 우선순위 제어 (알파벳 순서로 로드)
Z_ModLoader_P.pak        # 가장 먼저 로드 (최고 우선순위)
A_CoreMod_P.pak         # 두 번째로 로드
MyCustomMod_P.pak       # 일반 모드
01_FirstMod_P.pak       # 숫자 접두사 활용
99_LastMod_P.pak        # 가장 나중에 로드
```

**모드 충돌 해결 전략**:
```
1. 로더 우선순위: Z_ > A-Y > 숫자 > 일반명
2. 파일 덮어쓰기: 나중에 로드된 PAK이 우선
3. 모듈 분리: UI, 로직, 에셋을 별도 PAK으로 관리
4. 호환성 체크: 모드 로더가 충돌 감지 및 알림
```

#### 2. **스크립팅 모딩** (UE4SS 활용)
```lua
-- Lua 스크립트로 게임 로직 수정
local MyMod = {}

function MyMod:BeginPlay()
    -- 게임 시작 시 실행
    print("My Mod Loaded!")
end

return MyMod
```

#### 3. **메모리 패치 모딩** (고급)
```cpp
// Cheat Engine이나 C++ DLL을 사용한 메모리 조작
uintptr_t PlayerController = FindPattern("48 8B 05 ? ? ? ? 48 8B 88");
*(float*)(PlayerController + 0x1A4) = 999.0f; // HP 무한
```

---

## 🔧 UE4SS - 스크립팅 시스템

### 개요
UE4SS는 언리얼 엔진 4/5 게임을 위한 주입형 Lua 스크립팅 시스템입니다.

### 핵심 기능
- **Lua 스크립팅 API**: UE 객체 시스템 기반 모드 작성
- **Blueprint 모드 로딩**: 게임 파일 수정 없이 Blueprint 모드 자동 스폰
- **C++ 모딩 API**: UE 객체 시스템 기반 C++ 모드 작성
- **라이브 프로퍼티 에디터**: 실시간 객체 속성 검색, 편집, 모니터링

### 설치 방법
```bash
# 1. UE4SS 다운로드
# https://github.com/UE4SS-RE/RE-UE4SS/releases

# 2. 게임 폴더에 압축 해제
GameFolder/
├── GameName.exe
├── dwmapi.dll          # UE4SS 메인 DLL
├── UE4SS-settings.ini  # 설정 파일
└── Mods/               # 모드 폴더
    ├── mods.txt        # 모드 활성화 목록
    └── MyLuaMod/       # 개별 모드 폴더
        └── scripts/
            └── main.lua
```

### Lua 모드 작성 예제

#### 기본 구조
```lua
-- Mods/ExampleMod/scripts/main.lua

local ExampleMod = {}

-- 모드 초기화
function ExampleMod:init()
    print("Example Mod: Initializing...")
    
    -- 키 입력 이벤트 등록
    RegisterKeyBind(Key.F1, function()
        self:ToggleGodMode()
    end)
    
    -- 게임 이벤트 훅
    RegisterHook("/Script/Engine.GameModeBase:BeginPlay", function(Context)
        print("Game started!")
    end)
end

-- 무적 모드 토글
function ExampleMod:ToggleGodMode()
    local PlayerController = FindFirstOf("PlayerController")
    if not PlayerController:IsValid() then return end
    
    local PlayerPawn = PlayerController.Pawn
    if not PlayerPawn:IsValid() then return end
    
    -- 체력 컴포넌트 찾기
    local HealthComponent = PlayerPawn:GetComponentByClass("HealthComponent")
    if HealthComponent:IsValid() then
        -- 최대 체력으로 설정
        HealthComponent.CurrentHealth = HealthComponent.MaxHealth
        print("God Mode: Health restored!")
    end
end

-- 플레이어 텔레포트
function ExampleMod:TeleportPlayer(X, Y, Z)
    local PlayerController = FindFirstOf("PlayerController")
    if not PlayerController:IsValid() then return end
    
    local PlayerPawn = PlayerController.Pawn
    if PlayerPawn:IsValid() then
        -- 새 위치로 텔레포트
        local NewLocation = {
            X = X or 0,
            Y = Y or 0, 
            Z = Z or 0
        }
        PlayerPawn:K2_SetActorLocation(NewLocation, false, {}, false)
        print(string.format("Teleported to: %d, %d, %d", X, Y, Z))
    end
end

-- 아이템 스폰
function ExampleMod:SpawnItem(ItemClass, Quantity)
    local PlayerController = FindFirstOf("PlayerController")
    if not PlayerController:IsValid() then return end
    
    local World = PlayerController:GetWorld()
    local PlayerLocation = PlayerController.Pawn:K2_GetActorLocation()
    
    -- 아이템 스폰
    local SpawnedItem = World:SpawnActor(
        ItemClass,
        PlayerLocation,
        {Pitch = 0, Yaw = 0, Roll = 0},
        false,
        PlayerController.Pawn,
        PlayerController.Pawn,
        false
    )
    
    if SpawnedItem:IsValid() then
        print("Item spawned: " .. ItemClass:GetName())
    end
end

-- 모드 등록
RegisterMod("ExampleMod", ExampleMod)

return ExampleMod
```

#### 고급 기능 예제
```lua
-- 게임 상태 모니터링
local GameStateMonitor = {}

function GameStateMonitor:init()
    -- 타이머로 주기적 실행
    self.timer = CreateTimer(1.0, true, function()
        self:UpdateGameState()
    end)
end

function GameStateMonitor:UpdateGameState()
    local GameState = FindFirstOf("GameStateBase")
    if not GameState:IsValid() then return end
    
    -- 현재 플레이어 수 체크
    local PlayerArray = GameState.PlayerArray
    print("Current Players: " .. #PlayerArray)
    
    -- 게임 시간 출력
    local GameTime = GameState:GetServerWorldTimeSeconds()
    print("Game Time: " .. GameTime .. " seconds")
end

return GameStateMonitor
```

---

## 📦 PAK 기반 모드 로더 시스템

### 개요
커뮤니티 개발 모드 로더는 언리얼 엔진의 PAK 시스템을 활용하여 다중 모드를 안전하게 관리하는 솔루션입니다.

### Interpose Mod Loader 구조 분석

#### 핵심 컴포넌트
```
Z_ModLoader_P.pak           # 모드 시스템 핵심
├── ModManager              # 모드 로드/언로드 제어
├── PrioritySystem          # 로드 순서 관리
├── ConflictResolver        # 충돌 감지 및 해결
└── SafetyValidator         # 모드 안전성 검증

LogicMods/                  # 부가 기능 모듈
├── Z_DebugLog_P.pak       # 개발자 디버깅
├── Z_ModButtons_P.pak     # UI 제어 인터페이스
└── Z_ModListMenu_P.pak    # 모드 목록 관리
```

#### 고급 PAK 모딩 기법

**1. 우선순위 기반 로딩**
```bash
# 파일명으로 로드 순서 제어
Z_          # 최고 우선순위 (시스템 모드)
A-M_        # 높은 우선순위 (코어 모드)
N-Y_        # 중간 우선순위 (일반 모드)
00-99_      # 숫자 우선순위 (세밀한 제어)
[일반명]    # 기본 우선순위
```

**2. 모듈화된 모드 설계**
```
MyComplexMod/
├── Core_MyMod_P.pak        # 핵심 로직
├── UI_MyMod_P.pak          # 사용자 인터페이스
├── Assets_MyMod_P.pak      # 그래픽/사운드 에셋
└── Config_MyMod_P.pak      # 설정 파일
```

**3. 호환성 매트릭스**
```cpp
// 모드 로더 내부 호환성 체크 (의사코드)
class ModCompatibilityChecker {
    struct ModInfo {
        string name;
        string version;
        vector<string> dependencies;
        vector<string> conflicts;
        vector<string> overrides;
    };
    
    bool ValidateLoadOrder(vector<ModInfo> mods) {
        // 1. 의존성 해결
        // 2. 충돌 감지
        // 3. 오버라이드 검증
        // 4. 로드 순서 최적화
    }
};
```

### 실전 모드 개발 워크플로우

#### 1. **모드 개발 환경 구축**
```bash
# 1. 언리얼 에디터 설정
UE4Editor.exe -game -pak

# 2. 모드 프로젝트 구조
MyModProject/
├── Content/                # 에셋 파일들
│   ├── Blueprints/        # BP 로직
│   ├── Materials/         # 머티리얼
│   └── UI/               # 위젯
├── Config/               # 설정 파일
└── Scripts/              # 빌드 스크립트
```

#### 2. **PAK 패키징 과정**
```bash
# UnrealPak 도구를 사용한 패키징
UnrealPak.exe MyMod_P.pak -create=filelist.txt -compress

# filelist.txt 예제
"Content/Blueprints/BP_MyWeapon.uasset" "../../../MyGame/Content/Blueprints/Weapons/BP_MyWeapon.uasset"
"Content/Materials/M_MyTexture.uasset" "../../../MyGame/Content/Materials/M_MyTexture.uasset"
```

#### 3. **동적 모드 로딩 시스템**
```lua
-- UE4SS Lua를 통한 모드 상태 관리
local ModManager = {}

function ModManager:LoadMod(pakPath)
    -- PAK 파일 마운트
    local success = MountPak(pakPath)
    if success then
        -- 모드 초기화 스크립트 실행
        self:InitializeMod(pakPath)
        -- UI 업데이트
        self:UpdateModList()
    end
    return success
end

function ModManager:UnloadMod(modName)
    -- 안전한 모드 언로드
    self:CleanupModResources(modName)
    UnmountPak(modName)
    self:UpdateModList()
end
```

### 모드 로더의 기술적 혁신

#### 런타임 에셋 교체
```cpp
// C++ 코드로 런타임에 에셋 동적 교체
class RuntimeAssetSwapper {
public:
    void SwapTexture(const FString& OriginalPath, const FString& NewTexturePath) {
        UTexture2D* NewTexture = LoadAsset<UTexture2D>(NewTexturePath);
        if (NewTexture) {
            // 기존 텍스처 참조를 새 텍스처로 교체
            GetAssetRegistry().ReplaceAssetReference(OriginalPath, NewTexture);
        }
    }
    
    void SwapMaterial(const FString& MaterialPath, UMaterial* NewMaterial) {
        // 머티리얼 런타임 교체
        UMaterialInstanceDynamic* DynamicMaterial = 
            UMaterialInstanceDynamic::Create(NewMaterial, nullptr);
        ApplyMaterialToAllInstances(MaterialPath, DynamicMaterial);
    }
};
```

#### 메모리 효율적 모드 관리
```cpp
class ModMemoryManager {
private:
    TMap<FString, TSharedPtr<ModData>> LoadedMods;
    
public:
    void OptimizeMemoryUsage() {
        // 1. 사용하지 않는 모드 언로드
        UnloadUnusedMods();
        
        // 2. 중복 에셋 제거
        DeduplicateAssets();
        
        // 3. 메모리 압축
        CompressModData();
        
        // 4. 가비지 컬렉션 강제 실행
        GetEngine()->ForceGarbageCollection();
    }
};
```

---

## 🛠️ UETools - 개발자 도구

### 개요
UETools는 게임에 개발자 도구를 다시 가져다주는 강력한 모드입니다.

### 주요 기능
- **개발자 콘솔**: 커스텀 치트 명령어 제공
- **자유 카메라**: 2가지 타입의 자유 카메라 모드
- **게임 일시정지**: 개발자 도구를 통한 게임 정지
- **디버깅 도구**: 게임 상태 실시간 모니터링

### 설치 방법 (Lies of P 예제)
```bash
# 1. UETools 다운로드 및 압축 해제
# 2. 게임 설치 폴더로 이동
LiesofP/Content/Paks/

# 3. ~mods 폴더 생성 (없다면)
mkdir ~mods

# 4. UETools 파일을 ~mods 폴더에 복사
~mods/
├── UETools_P.pak
└── UETools_P.sig
```

### 콘솔 명령어 예제
```bash
# UETools 도움말
UETools_Help

# 자유 카메라 활성화
ToggleDebugCamera

# 플레이어 비행 모드
fly

# 충돌 무시 모드 (노클립)
ghost

# 무적 모드
god

# 게임 일시정지
pause

# 텔레포트 (좌표)
teleport 100 200 300

# 아이템 생성
summon ItemBP_HealthPotion_C

# 게임 속도 조절
slomo 0.5  # 50% 속도
slomo 2.0  # 200% 속도
```

---

## 🎨 UI 모딩 - ImGui 통합

### 개요
ImGui(Immediate Mode GUI)를 언리얼 엔진에 통합하여 커스텀 UI를 만드는 기법입니다.

### UnrealImGui 플러그인

#### 설치 방법
```bash
# 1. UnrealImGui 플러그인 다운로드
# https://github.com/segross/UnrealImGui

# 2. 프로젝트 플러그인 폴더에 복사
[ProjectRoot]/Plugins/ImGui/

# 3. 프로젝트 재빌드
```

#### C++ 코드 예제
```cpp
// MyImGuiWidget.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImGui.h"
#include "MyImGuiWidget.generated.h"

UCLASS()
class MYGAME_API AMyImGuiWidget : public AActor
{
    GENERATED_BODY()
    
public:
    AMyImGuiWidget();
    
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
private:
    void DrawImGuiWindow();
    
    bool bShowDemoWindow = true;
    bool bShowPlayerStats = true;
    
    // 플레이어 스탯
    float PlayerHealth = 100.0f;
    float PlayerMana = 50.0f;
    int32 PlayerLevel = 1;
};
```

```cpp
// MyImGuiWidget.cpp
#include "MyImGuiWidget.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

AMyImGuiWidget::AMyImGuiWidget()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AMyImGuiWidget::BeginPlay()
{
    Super::BeginPlay();
}

void AMyImGuiWidget::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    DrawImGuiWindow();
}

void AMyImGuiWidget::DrawImGuiWindow()
{
    // 메인 메뉴 바
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Player Stats", nullptr, &bShowPlayerStats);
            ImGui::MenuItem("Demo Window", nullptr, &bShowDemoWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    // 플레이어 스탯 윈도우
    if (bShowPlayerStats)
    {
        ImGui::Begin("Player Stats", &bShowPlayerStats);
        
        // 체력 바
        ImGui::Text("Health: %.1f/100.0", PlayerHealth);
        ImGui::ProgressBar(PlayerHealth / 100.0f, ImVec2(-1, 0));
        
        // 마나 바
        ImGui::Text("Mana: %.1f/100.0", PlayerMana);
        ImGui::ProgressBar(PlayerMana / 100.0f, ImVec2(-1, 0));
        
        // 레벨 정보
        ImGui::Text("Level: %d", PlayerLevel);
        
        // 치트 버튼들
        ImGui::Separator();
        if (ImGui::Button("Full Health"))
        {
            PlayerHealth = 100.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Full Mana"))
        {
            PlayerMana = 100.0f;
        }
        
        if (ImGui::Button("Level Up"))
        {
            PlayerLevel++;
        }
        
        ImGui::End();
    }
    
    // ImGui 데모 윈도우
    if (bShowDemoWindow)
    {
        ImGui::ShowDemoWindow(&bShowDemoWindow);
    }
}
```

### 실시간 에디터 예제
```cpp
// 실시간 게임 오브젝트 에디터
void AMyImGuiWidget::DrawObjectEditor()
{
    static UObject* SelectedObject = nullptr;
    
    ImGui::Begin("Object Editor");
    
    // 오브젝트 선택
    if (ImGui::Button("Select Player"))
    {
        SelectedObject = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }
    
    if (SelectedObject && IsValid(SelectedObject))
    {
        ImGui::Text("Selected: %s", *SelectedObject->GetName());
        
        // 위치 편집
        if (APawn* Pawn = Cast<APawn>(SelectedObject))
        {
            FVector Location = Pawn->GetActorLocation();
            float Pos[3] = { Location.X, Location.Y, Location.Z };
            
            if (ImGui::DragFloat3("Position", Pos, 1.0f))
            {
                Pawn->SetActorLocation(FVector(Pos[0], Pos[1], Pos[2]));
            }
            
            // 회전 편집
            FRotator Rotation = Pawn->GetActorRotation();
            float Rot[3] = { Rotation.Pitch, Rotation.Yaw, Rotation.Roll };
            
            if (ImGui::DragFloat3("Rotation", Rot, 1.0f))
            {
                Pawn->SetActorRotation(FRotator(Rot[0], Rot[1], Rot[2]));
            }
        }
    }
    
    ImGui::End();
}
```

---

## 🎭 Blueprint 모딩

### Blueprint 에셋 구조
```
MyBlueprintMod/
├── Content/
│   ├── Blueprints/
│   │   ├── BP_CustomWeapon.uasset      # 커스텀 무기
│   │   ├── BP_CustomCharacter.uasset   # 커스텀 캐릭터
│   │   └── BP_CustomGameMode.uasset    # 게임 모드 수정
│   ├── Materials/
│   │   ├── M_CustomSword.uasset        # 무기 머티리얼
│   │   └── M_CustomArmor.uasset        # 방어구 머티리얼
│   ├── Meshes/
│   │   ├── SM_CustomSword.uasset       # 무기 메시
│   │   └── SM_CustomShield.uasset      # 방패 메시
│   └── UI/
│       ├── WBP_CustomHUD.uasset        # 커스텀 HUD
│       └── WBP_InventoryMenu.uasset    # 인벤토리 UI
└── Config/
    └── DefaultGame.ini                  # 게임 설정 오버라이드
```

### Blueprint 스크립팅 예제

#### 커스텀 무기 Blueprint
```cpp
// BP_CustomWeapon의 로직 (의사코드)
class BP_CustomWeapon : public AWeapon
{
    // 변수들
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CustomDamage = 150.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)  
    float CriticalChance = 0.15f;
    
    // 공격 함수
    UFUNCTION(BlueprintCallable)
    void CustomAttack()
    {
        // 크리티컬 히트 계산
        bool bIsCritical = FMath::RandRange(0.0f, 1.0f) < CriticalChance;
        float FinalDamage = bIsCritical ? CustomDamage * 2.0f : CustomDamage;
        
        // 데미지 적용
        if (TargetActor)
        {
            ApplyDamage(TargetActor, FinalDamage);
            
            // 이펙트 재생
            if (bIsCritical)
            {
                PlayCriticalEffect();
            }
        }
    }
};
```

#### 커스텀 UI Widget
```cpp
// WBP_CustomHUD의 로직 (의사코드)
class WBP_CustomHUD : public UUserWidget
{
    // UI 요소들
    UPROPERTY(meta = (BindWidget))
    UProgressBar* HealthBar;
    
    UPROPERTY(meta = (BindWidget))
    UProgressBar* ManaBar;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* LevelText;
    
    // 업데이트 함수
    UFUNCTION(BlueprintImplementableEvent)
    void UpdatePlayerStats()
    {
        if (PlayerCharacter)
        {
            // 체력 바 업데이트
            float HealthPercent = PlayerCharacter->GetHealth() / PlayerCharacter->GetMaxHealth();
            HealthBar->SetPercent(HealthPercent);
            
            // 마나 바 업데이트  
            float ManaPercent = PlayerCharacter->GetMana() / PlayerCharacter->GetMaxMana();
            ManaBar->SetPercent(ManaPercent);
            
            // 레벨 텍스트 업데이트
            FString LevelString = FString::Printf(TEXT("Level: %d"), PlayerCharacter->GetLevel());
            LevelText->SetText(FText::FromString(LevelString));
        }
    }
};
```

---

## 🎯 실전 예제: Lies of P

### 게임 정보
- **엔진**: Unreal Engine 4.27
- **장르**: 소울라이크 액션 RPG
- **모딩 지원**: UE4SS, UETools, PAK 모딩

### 주요 모드 종류

#### 1. **Interpose Mod Loader - PAK 기반 모딩 시스템**

**개요**:
Interpose는 Lies of P를 위한 커뮤니티 개발 모드 로더로, 복수의 PAK 모드를 안전하게 로드하고 관리하는 시스템입니다.

**파일 구조**:
```
Interpose Lies of P Mod Loader/
├── ~mods/
│   └── Z_ModLoader_P.pak          # 핵심 모드 로더
└── LogicMods/
    ├── Z_DebugLog_P.pak           # 디버그 로그 기능
    ├── Z_ModButtons_P.pak         # 모드 제어 UI
    └── Z_ModListMenu_P.pak        # 모드 목록 메뉴
```

**설치 및 사용법**:
```bash
# 1. 게임 폴더 내 PAK 디렉토리로 이동
cd "LiesofP\Content\Paks"

# 2. 모드 로더 설치
~mods\Z_ModLoader_P.pak           # 반드시 먼저 설치

# 3. 로직 모드 설치 (선택사항)
LogicMods\Z_DebugLog_P.pak        # 디버그 기능
LogicMods\Z_ModButtons_P.pak      # UI 제어
LogicMods\Z_ModListMenu_P.pak     # 모드 관리
```

**핵심 기능**:
- **모드 우선순위**: Z_ 접두사로 로드 순서 제어
- **안전한 로딩**: 게임 크래시 없이 다중 모드 지원
- **UI 통합**: 게임 내에서 모드 상태 확인
- **디버그 지원**: 모드 개발자를 위한 로그 시스템

**AISpawner.pak 예제**:
```bash
# AISpawner.pak은 Interpose와 함께 사용 가능한 모드
# 기능: 게임 내 AI 적 스폰 제어
~mods\
├── Z_ModLoader_P.pak             # 모드 로더 (필수)
├── AISpawner.pak                 # AI 스폰 모드
└── [기타 모드들...]
```

**개발자 관점에서의 학습 가치**:
- **PAK 우선순위 시스템**: UE4의 PAK 로딩 메커니즘 이해
- **모듈화된 설계**: 로더와 기능 모드의 분리
- **UI/UX 통합**: 게임 내 모드 관리 인터페이스
- **호환성 관리**: 다중 모드 충돌 방지 기법

#### 2. **UETools 개발자 콘솔**

**설치 방법**:
```bash
# 1. 게임 설치 폴더 이동
cd "LiesofP\Content\Paks"

# 2. ~mods 폴더 생성 (없는 경우)
mkdir ~mods

# 3. UETools 파일 복사
~mods\
├── UETools_P.pak
└── UETools_P.sig
```

**주요 기능**:
- **스크린샷 시스템**: F1-F3 키로 다양한 해상도 스크린샷
- **디버그 도구**: 게임 내부 상태 시각화
- **커스텀 콘솔 명령어**: UE4SS와 통합된 치트 명령어
- **키보드 단축키**: 개발자 친화적 단축키 시스템

**단축키 매핑**:
```
F1      : 스크린샷 (UI 없음)
F2      : 스크린샷 (UI 포함)  
F3      : 고해상도 스크린샷 (기본 3840x2160)
PageUp  : 다음 디버그 타겟
PageDown: 이전 디버그 타겟
End     : 디버그 통계 토글
Divide  : 이전 뷰모드
Multiply: 다음 뷰모드
```

**Engine.ini 설정 예제**:
```ini
[/Game/UETools_Implemintation/Progressive/Settings.Settings_C]
# 핵심 설정
CoreValidationDelay=0.2
bCreateDebugMappings=True

# 단축키 커스터마이징
DebugMapping_Screenshot=F1
DebugMapping_ScreenshotUI=F2
DebugMapping_HighResScreenshot=F3
HighResScreenshotResolution=3840x2160

# 자동 실행 명령어 (최대 12개)
AutoExecCommand_01=UETools_Help
AutoExecCommand_02=god
AutoExecCommand_03=

# 모딩 지원 설정
bModdingSupportEnabled=False
ModdingActorPath_01=NONE

# 콘솔 및 치트 관리자
bConstructConsole=True
bCreateConsoleMappings=True
bConstructCheatManager=True
```

**주요 콘솔 명령어**:
```bash
UETools_Help           # 도움말 및 명령어 목록
ToggleDebugCamera     # 자유 카메라 모드
fly                   # 비행 모드
ghost                 # 노클립 모드  
god                   # 무적 모드
slomo 0.1            # 슬로우 모션 (0.1 = 10% 속도)
pause                # 게임 일시정지
teleport 100 200 300 # 지정 좌표로 텔레포트
```

#### 2. **UE4SS Lua 스크립팅**
```lua
-- LiesOfP_GodMode.lua
local LiesOfPMod = {}

function LiesOfPMod:init()
    print("Lies of P: God Mode Mod Loaded")
    
    -- F1키로 무적 모드 토글
    RegisterKeyBind(Key.F1, function()
        self:ToggleGodMode()
    end)
    
    -- F2키로 무한 스태미나
    RegisterKeyBind(Key.F2, function()
        self:ToggleInfiniteStamina()
    end)
end

function LiesOfPMod:ToggleGodMode()
    local PlayerCharacter = FindFirstOf("LoPBaseCharacter")
    if not PlayerCharacter:IsValid() then return end
    
    -- 체력을 최대치로 설정
    local HealthComponent = PlayerCharacter:GetComponentByClass("HealthComponent")
    if HealthComponent:IsValid() then
        HealthComponent.CurrentHealth = HealthComponent.MaxHealth
        print("God Mode: Health restored!")
    end
end

function LiesOfPMod:ToggleInfiniteStamina()
    local PlayerController = FindFirstOf("LoPPlayerController")
    if not PlayerController:IsValid() then return end
    
    local PlayerCharacter = PlayerController.Pawn
    if PlayerCharacter:IsValid() then
        -- 스태미나 컴포넌트 찾기
        local StaminaComponent = PlayerCharacter:GetComponentByClass("StaminaComponent")
        if StaminaComponent:IsValid() then
            StaminaComponent.CurrentStamina = StaminaComponent.MaxStamina
            print("Infinite Stamina activated!")
        end
    end
end

return LiesOfPMod
```

#### 3. **커스텀 UI 모드**
```lua
-- LiesOfP_CustomUI.lua
local UIHelper = {}

function UIHelper:init()
    -- 커스텀 HUD 생성
    self:CreateCustomHUD()
end

function UIHelper:CreateCustomHUD()
    -- 기존 HUD 위젯 찾기
    local MainHUD = FindFirstOf("WBP_MainHUD")
    if not MainHUD:IsValid() then return end
    
    -- 새 정보 패널 추가
    self:AddInfoPanel(MainHUD)
end

function UIHelper:AddInfoPanel(ParentWidget)
    -- 플레이어 위치 표시 패널
    local InfoPanel = CreateWidget("WBP_InfoPanel")
    ParentWidget:AddChild(InfoPanel)
    
    -- 주기적으로 정보 업데이트
    CreateTimer(0.1, true, function()
        self:UpdateInfoPanel(InfoPanel)
    end)
end

function UIHelper:UpdateInfoPanel(Panel)
    local PlayerCharacter = FindFirstOf("LoPBaseCharacter")
    if not PlayerCharacter:IsValid() then return end
    
    -- 플레이어 위치 정보
    local Location = PlayerCharacter:K2_GetActorLocation()
    local LocationText = string.format("Position: %.0f, %.0f, %.0f", 
                                      Location.X, Location.Y, Location.Z)
    
    -- UI 텍스트 업데이트
    Panel:SetLocationText(LocationText)
    
    -- 체력 정보
    local HealthComponent = PlayerCharacter:GetComponentByClass("HealthComponent")
    if HealthComponent:IsValid() then
        local HealthText = string.format("HP: %.0f/%.0f", 
                                        HealthComponent.CurrentHealth,
                                        HealthComponent.MaxHealth)
        Panel:SetHealthText(HealthText)
    end
end

return UIHelper
```

### 에셋 모딩 예제
```bash
# 텍스처 교체 모드
LiesofP_TextureMod/
├── Content/
│   ├── Characters/
│   │   └── Pinocchio/
│   │       ├── Materials/
│   │       │   └── M_Pinocchio_Body.uasset    # 캐릭터 머티리얼
│   │       └── Textures/
│   │           ├── T_Pinocchio_Diffuse.uasset # 디퓨즈 텍스처
│   │           └── T_Pinocchio_Normal.uasset  # 노멀 맵
│   └── UI/
│       ├── Textures/
│       │   └── T_CustomLogo.uasset            # 커스텀 로고
│       └── Widgets/
│           └── WBP_MainMenu.uasset            # 메인 메뉴 수정
└── LiesofP_TextureMod.pak                     # 패키지된 모드
```

## 🔧 모딩 도구 및 유틸리티

### UE Viewer (UModel)
- **용도**: .pak 파일 추출 및 에셋 뷰어
- **지원 형식**: 텍스처, 메시, 애니메이션
- **사용법**: `umodel.exe GameName.pak`

### FModel
- **용도**: 최신 언리얼 엔진 에셋 뷰어
- **특징**: GUI 기반, 고급 필터링
- **지원**: UE 4.0 ~ 5.x

### Unreal Engine 에디터
- **용도**: 프로페셔널 모드 개발
- **요구사항**: C++ 프로젝트 필요
- **장점**: 완전한 개발 환경

---

## 📚 추천 학습 자료

### 공식 문서
- [UE4SS Documentation](https://docs.ue4ss.com/)
- [Unreal Engine Modding Tools](https://github.com/Buckminsterfullerene02/UE-Modding-Tools)
- [UnrealImGui Plugin](https://github.com/segross/UnrealImGui)

### 커뮤니티
- **Discord**: Unreal Engine Modding Community
- **Reddit**: r/unrealengine, r/gamedev
- **GitHub**: 다양한 UE 모딩 프로젝트들

### 비디오 자료
- **YouTube**: "Unreal Engine Modding Tutorial"
- **Twitch**: 실시간 모딩 스트림들

---

**💡 핵심 팁**:

1. **시작하기**: UE4SS로 간단한 Lua 스크립트부터
2. **점진적 발전**: 스크립팅 → Blueprint → C++
3. **커뮤니티 활용**: 다른 모더들과 지식 공유
4. **실험 정신**: 다양한 게임에서 기법 시도

언리얼 엔진 모딩은 강력하고 유연한 플랫폼을 제공하므로, 체계적으로 학습하면 놀라운 결과물을 만들 수 있습니다!