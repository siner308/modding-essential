# 게임 모딩 접근법 비교 가이드

게임 모딩에는 다양한 접근 방식이 있습니다. 각각의 장단점과 적용 사례를 통해 최적의 모딩 방법을 선택할 수 있도록 안내합니다.

## 📊 모딩 접근법 분류

### 1. **공식 모딩 지원** (Official Modding Support)
게임 개발사가 제공하는 공식 도구와 API를 사용하는 방식

### 2. **스크립트 기반 모딩** (Script-based Modding)
게임 내장 스크립트 엔진을 활용하는 방식

### 3. **프레임워크 기반 모딩** (Framework-based Modding)
커뮤니티가 개발한 모딩 프레임워크를 사용하는 방식

### 4. **바이너리 모딩** (Binary Modding)
실행 파일을 직접 수정하는 방식 ← **이 저장소의 주 학습 대상**

## 🎮 게임별 모딩 사례 분석

---

## 1. Steam Workshop - 공식 모딩 플랫폼

### 개요
Steam에서 제공하는 공식 모드 배포 및 관리 시스템

### 기술적 특징
```cpp
// Steam Workshop API 사용 예제
#include "steam_api.h"

class WorkshopManager {
public:
    void InitializeWorkshop() {
        // ISteamUGC 인터페이스 초기화
        SteamUGC()->SetReturnOnlyIDs(false);
        SteamUGC()->SetReturnChildren(true);
    }
    
    void CreateWorkshopItem() {
        // 새 워크샵 아이템 생성
        SteamAPICall_t hAPICall = SteamUGC()->CreateItem(
            SteamUtils()->GetAppID(), 
            k_EWorkshopFileTypeCommunity
        );
    }
};
```

### 장점
- ✅ **안전성**: 공식 지원으로 안정적
- ✅ **배포 용이**: 자동 업데이트 및 배포
- ✅ **커뮤니티**: 대규모 사용자 기반
- ✅ **통합**: 게임 내 자동 연동

### 단점
- ❌ **제한적**: 개발사가 허용한 범위 내에서만
- ❌ **의존성**: Steam 플랫폼에 종속
- ❌ **규제**: 엄격한 콘텐츠 가이드라인

### 주요 게임 예시
- **Cities: Skylines**: 에셋 및 모드 지원
- **Garry's Mod**: 광범위한 커뮤니티 콘텐츠
- **Total War 시리즈**: 맵, 유닛, 캠페인 모드

---

## 2. World of Warcraft - Lua 애드온 시스템

### 개요
Lua 스크립트와 XML UI를 사용하는 공식 애드온 시스템

### 기술적 특징
```lua
-- WoW 애드온 기본 구조
local MyAddon = CreateFrame("Frame")

-- 이벤트 등록
MyAddon:RegisterEvent("ADDON_LOADED")
MyAddon:RegisterEvent("PLAYER_LOGIN")

-- 이벤트 핸들러
MyAddon:SetScript("OnEvent", function(self, event, ...)
    if event == "ADDON_LOADED" then
        local addonName = ...
        if addonName == "MyAddon" then
            self:Initialize()
        end
    elseif event == "PLAYER_LOGIN" then
        self:OnPlayerLogin()
    end
end)

-- WoW API 사용 예제
function MyAddon:Initialize()
    -- 플레이어 정보 조회
    local playerName = UnitName("player")
    local playerLevel = UnitLevel("player")
    
    -- UI 프레임 생성
    local frame = CreateFrame("Frame", "MyAddonFrame", UIParent)
    frame:SetSize(200, 100)
    frame:SetPoint("CENTER")
    
    -- 채팅 출력
    print("MyAddon loaded for " .. playerName .. " (Level " .. playerLevel .. ")")
end
```

### 애드온 파일 구조
```
MyAddon/
├── MyAddon.toc          # 애드온 메타데이터
├── MyAddon.lua          # 메인 로직
├── MyAddon.xml          # UI 정의
└── Localization/        # 다국어 지원
    ├── enUS.lua
    └── koKR.lua
```

### .toc 파일 예제
```ini
## Interface: 100207
## Title: My Addon
## Notes: Example addon for learning
## Author: YourName
## Version: 1.0
## SavedVariables: MyAddonDB

MyAddon.xml
MyAddon.lua
```

### 장점
- ✅ **공식 지원**: 블리자드 공식 API
- ✅ **풍부한 API**: 게임 내 거의 모든 정보 접근 가능
- ✅ **UI 커스터마이징**: 완전한 인터페이스 재구성 가능
- ✅ **커뮤니티**: 대규모 애드온 생태계

### 단점
- ❌ **제한된 권한**: 게임 로직 수정 불가
- ❌ **API 의존성**: 게임 업데이트 시 호환성 문제
- ❌ **Lua 학습**: 별도 스크립트 언어 학습 필요

---

## 3. Skyrim - Creation Kit & Papyrus

### 개요
베데스다가 제공하는 공식 모딩 도구와 Papyrus 스크립트 언어

### 기술적 특징
```papyrus
; Papyrus 스크립트 예제
Scriptname MyCustomSpell extends ActiveMagicEffect

Event OnEffectStart(Actor akTarget, Actor akCaster)
    ; 스펠 시작 시 실행
    Debug.MessageBox("Custom spell activated!")
    
    ; 플레이어 체력 회복
    if akTarget == Game.GetPlayer()
        akTarget.RestoreActorValue("Health", 50)
        Debug.Notification("Health restored!")
    endif
EndEvent

Event OnEffectFinish(Actor akTarget, Actor akCaster)
    ; 스펠 종료 시 실행
    Debug.Notification("Custom spell effect ended")
EndEvent
```

### Quest 스크립트 예제
```papyrus
Scriptname MyQuestScript extends Quest

Event OnInit()
    ; 퀘스트 초기화
    RegisterForSingleUpdate(1.0)
EndEvent

Event OnUpdate()
    ; 정기적으로 실행되는 로직
    Actor player = Game.GetPlayer()
    
    if player.GetCurrentLocation().GetName() == "Whiterun"
        ; 화이트런에 있을 때만 실행
        SetStage(10)
    endif
    
    RegisterForSingleUpdate(5.0) ; 5초 후 다시 실행
EndEvent
```

### 장점
- ✅ **강력한 도구**: Creation Kit으로 레벨, NPC, 아이템 등 모든 요소 편집
- ✅ **스크립팅**: Papyrus로 복잡한 로직 구현
- ✅ **에셋 지원**: 3D 모델, 텍스처, 사운드 교체/추가
- ✅ **광범위한 수정**: 게임 메커니즘 근본적 변경 가능

### 단점
- ❌ **복잡성**: 학습 곡선이 가파름
- ❌ **성능**: Papyrus 스크립트 성능 제한
- ❌ **호환성**: 모드 간 충돌 문제

---

## 4. Minecraft - Forge & Fabric (Java)

### 개요
Java 기반의 강력한 모딩 플랫폼들

### Forge 모딩 예제
```java
// Forge 모드 메인 클래스
@Mod("examplemod")
public class ExampleMod {
    public static final String MODID = "examplemod";
    private static final Logger LOGGER = LogUtils.getLogger();

    public ExampleMod() {
        // 모드 이벤트 등록
        IEventBus modEventBus = FMLJavaModLoadingContext.get().getModEventBus();
        modEventBus.addListener(this::commonSetup);
        
        // 게임 이벤트 등록
        MinecraftForge.EVENT_BUS.register(this);
    }

    private void commonSetup(final FMLCommonSetupEvent event) {
        LOGGER.info("Example mod setup complete!");
    }

    // 커스텀 블록 추가
    @SubscribeEvent
    public static void onBlocksRegistry(final RegistryEvent.Register<Block> blockRegistryEvent) {
        blockRegistryEvent.getRegistry().register(
            new Block(BlockBehaviour.Properties.of(Material.STONE))
                .setRegistryName("examplemod", "example_block")
        );
    }
}
```

### Fabric 모딩 예제
```java
// Fabric 모드 초기화
public class ExampleMod implements ModInitializer {
    public static final String MOD_ID = "examplemod";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    @Override
    public void onInitialize() {
        // 커스텀 아이템 등록
        Registry.register(
            Registries.ITEM,
            new Identifier(MOD_ID, "example_item"),
            new Item(new FabricItemSettings())
        );

        // 이벤트 콜백 등록
        AttackBlockCallback.EVENT.register((player, world, hand, pos, direction) -> {
            LOGGER.info("Player attacked block at {}", pos);
            return ActionResult.PASS;
        });
    }
}
```

### mod.json (Fabric 메타데이터)
```json
{
  "schemaVersion": 1,
  "id": "examplemod",
  "version": "1.0.0",
  "name": "Example Mod",
  "description": "An example mod",
  "authors": ["YourName"],
  "license": "MIT",
  "icon": "assets/examplemod/icon.png",
  
  "environment": "*",
  "entrypoints": {
    "main": ["com.example.ExampleMod"]
  },
  "mixins": ["examplemod.mixins.json"],
  
  "depends": {
    "fabricloader": ">=0.14.0",
    "minecraft": "~1.20.1",
    "java": ">=17",
    "fabric-api": "*"
  }
}
```

### 장점
- ✅ **Java 기반**: 강력하고 유연한 프로그래밍 언어
- ✅ **광범위한 수정**: 게임 로직 완전 변경 가능
- ✅ **활발한 커뮤니티**: 방대한 자료와 라이브러리
- ✅ **버전 호환성**: Fabric은 빠른 업데이트 지원

### 단점
- ❌ **Java 학습**: 프로그래밍 지식 필수
- ❌ **복잡성**: 객체지향 개념 이해 필요
- ❌ **디버깅**: 모드 충돌 및 오류 해결 어려움

---

## 5. Unity 게임 - BepInEx 프레임워크

### 개요
Unity 엔진 게임을 위한 커뮤니티 개발 모딩 프레임워크

### 기술적 특징
```csharp
// BepInEx 플러그인 예제
using BepInEx;
using BepInEx.Configuration;
using HarmonyLib;
using UnityEngine;

[BepInPlugin("com.example.myplugin", "My Plugin", "1.0.0")]
public class MyPlugin : BaseUnityPlugin
{
    private ConfigEntry<bool> enableFeature;
    
    void Awake()
    {
        // 설정 바인딩
        enableFeature = Config.Bind("General", "EnableFeature", true, 
                                   "Enable the custom feature");
        
        // Harmony 패치 적용
        var harmony = new Harmony("com.example.myplugin");
        harmony.PatchAll();
        
        Logger.LogInfo("Plugin loaded successfully!");
    }
}

// Harmony를 사용한 메서드 패치
[HarmonyPatch(typeof(PlayerController), "Update")]
public class PlayerControllerPatch
{
    static void Prefix(PlayerController __instance)
    {
        // 원본 메서드 실행 전에 실행
        if (Input.GetKeyDown(KeyCode.F1))
        {
            __instance.transform.position = Vector3.zero;
        }
    }
    
    static void Postfix(PlayerController __instance)
    {
        // 원본 메서드 실행 후에 실행
        Debug.Log($"Player position: {__instance.transform.position}");
    }
}
```

### 런타임 IL 패치 예제
```csharp
// IL 코드 직접 조작
[HarmonyPatch(typeof(GameManager), "TakeDamage")]
public class InvincibilityPatch
{
    static IEnumerable<CodeInstruction> Transpiler(IEnumerable<CodeInstruction> instructions)
    {
        var codes = new List<CodeInstruction>(instructions);
        
        for (int i = 0; i < codes.Count; i++)
        {
            // 데미지 적용 코드를 찾아서 0으로 변경
            if (codes[i].opcode == OpCodes.Ldarg_1) // 데미지 파라미터 로드
            {
                codes[i] = new CodeInstruction(OpCodes.Ldc_I4_0); // 0으로 교체
                break;
            }
        }
        
        return codes.AsEnumerable();
    }
}
```

### 장점
- ✅ **Unity 전용**: Unity 엔진 게임에 특화
- ✅ **런타임 패치**: 게임 파일 수정 없이 메모리에서 패치
- ✅ **C# 기반**: .NET 생태계 활용 가능
- ✅ **Harmony 통합**: 강력한 메서드 후킹 라이브러리

### 단점
- ❌ **Unity 전용**: 다른 엔진 게임에 사용 불가
- ❌ **IL2CPP 제한**: IL2CPP 빌드에서 제한적 기능
- ❌ **업데이트 취약**: 게임 업데이트 시 패치 위치 변경

---

## 6. FromSoftware 게임 - 커뮤니티 도구 생태계

### 개요
FromSoftware 게임들(Dark Souls, Elden Ring, Sekiro)은 Dantelion 엔진을 사용하며, 전문적인 커뮤니티 개발 도구들로 모딩이 가능합니다.

### 기술적 특징
```bash
# Smithbox를 사용한 파라미터 편집
NpcParam 편집:
├── HP: 100 → 500 (체력 5배 증가)
├── Attack: 50 → 25 (공격력 절반)
├── Soul Drop: 25 → 100 (소울 4배)
└── AI Think: 123000 → 456000 (AI 변경)

# DSMapStudio 맵 편집
Map Entity 추가:
1. 빈 엔티티 슬롯 선택
2. Entity ID 설정 (적/아이템 등)
3. 좌표 입력 (X: 100, Y: 0, Z: 200)
4. 회전값 설정 (0, 90, 0)
```

### 워크플로우 예제
```bash
# 1. 파일 추출 (WitchyBND)
WitchyBND.exe Data0.bdt

# 2. 파라미터 편집 (Smithbox)
Smithbox → Param Editor → NpcParam 수정

# 3. 맵 편집 (Smithbox/DSMapStudio)  
Map Editor → 적 배치 변경

# 4. 모드 패키징 (Mod Engine 2)
모드 폴더에 수정 파일 배치
modengine2_launcher.exe 실행
```

### 장점
- ✅ **전문 도구**: 상용 수준의 편집 환경
- ✅ **광범위한 수정**: 맵, 적, 아이템, 밸런스 모든 것
- ✅ **안전성**: 원본 파일 보호 시스템
- ✅ **활발한 커뮤니티**: SoulsModding 위키, Discord

### 단점
- ❌ **학습 곡선**: 독특한 파일 구조와 도구들
- ❌ **게임별 차이**: DS2는 별도 도구 필요
- ❌ **온라인 제한**: EAC로 인한 오프라인 전용
- ❌ **역공학 의존**: 공식 문서 없음

### 학습 가치
- 🎓 **커뮤니티 도구**: 역공학 기반 도구 개발 이해
- 🎓 **게임 엔진**: Dantelion 엔진의 독특한 구조
- 🎓 **모드 생태계**: 전문화된 모딩 커뮤니티 경험
- 🎓 **파일 포맷**: 바이너리 파일 구조 분석 능력

---

## 7. UETools - 언리얼 엔진 개발자 도구

### 개요
UETools는 언리얼 엔진 게임에 개발자 콘솔과 디버깅 기능을 다시 가져다주는 PAK 기반 모드입니다.

### 기술적 특징
```ini
# Engine.ini 설정을 통한 모드 제어
[/Game/UETools_Implemintation/Progressive/Settings.Settings_C]
# 스크린샷 해상도 설정
HighResScreenshotResolution=3840x2160

# 자동 실행 명령어 (게임 시작 시 자동 실행)
AutoExecCommand_01=UETools_Help
AutoExecCommand_02=god
AutoExecCommand_03=fly

# 커스텀 키 매핑
DebugMapping_Screenshot=F1
DebugMapping_ToggleFullScreen=F11
DebugMapping_PreviousViewMode=Divide
```

### 실전 활용 예제
```bash
# 개발/테스트용 명령어 조합
UETools_Help              # 사용 가능한 모든 명령어 확인
god                       # 무적 모드로 안전한 탐험
fly                       # 맵 전체 탐험
ghost                     # 벽 통과로 숨겨진 영역 접근
slomo 0.1                 # 슬로우 모션으로 세밀한 관찰
pause                     # 일시정지하여 상황 분석

# 스크린샷/영상 제작용
ToggleDebugCamera         # 자유 카메라로 원하는 앵글
ShowDebug None            # UI 제거하여 깔끔한 화면
HighResScreenshot         # 고해상도 스크린샷 촬영
```

### 모딩 작업에서의 활용
```bash
# 게임 분석용
ShowDebug AI              # AI 디버그 정보 표시
ShowDebug Collision       # 충돌 박스 시각화  
ShowDebug Rendering       # 렌더링 상태 확인
stat fps                  # 성능 모니터링
stat unit                 # CPU/GPU 사용량 확인

# 레벨 디자인 분석
teleport X Y Z            # 특정 위치로 즉시 이동
fly                       # 레벨 구조 파악
ghost                     # 레벨 경계 확인
```

### 장점
- ✅ **설치 간편**: PAK 파일만 복사하면 완료
- ✅ **즉시 사용**: 게임 재시작 없이 바로 활용
- ✅ **안전성**: 게임 파일 직접 수정 없음
- ✅ **개발자 도구**: UE 엔진의 내장 기능 활용
- ✅ **커스터마이징**: Engine.ini로 상세 설정 가능

### 단점
- ❌ **UE 전용**: 언리얼 엔진 게임에만 적용
- ❌ **제한적 수정**: 콘솔 명령어 범위 내에서만 가능
- ❌ **업데이트 의존**: 게임 업데이트 시 재설치 필요
- ❌ **온라인 제한**: 멀티플레이어에서 사용 제한

### 학습 가치
- 🎓 **UE 엔진 이해**: 언리얼 엔진의 내부 구조 학습
- 🎓 **콘솔 명령어**: UE 개발자 도구 활용법
- 🎓 **PAK 시스템**: 언리얼 에셋 패키징 시스템 이해
- 🎓 **설정 파일**: Engine.ini 구조와 활용법

---

## 7. 바이너리 모딩 - 직접 메모리 조작

### 개요
실행 파일이나 메모리를 직접 수정하는 방식 (이 저장소의 주 학습 대상)

### 기술적 특징
```cpp
// EldenRing PauseTheGame 예제 (재검토)
class BinaryModder {
private:
    uintptr_t gameBase;
    std::string targetAOB = "0f 84 ? ? ? ? c6 ? ? ? ? ? 00";
    
public:
    bool ApplyPatch() {
        // 1. AOB 스캔으로 패치 위치 찾기
        uintptr_t patchAddr = AobScan(targetAOB);
        if (patchAddr == 0) return false;
        
        // 2. 메모리 보호 해제
        ToggleMemoryProtection(false, patchAddr, 2);
        
        // 3. JE(0x84) → JNE(0x85) 패치 적용
        bool success = ReplaceBytes(patchAddr + 1, "84", "85");
        
        // 4. 메모리 보호 복원
        ToggleMemoryProtection(true, patchAddr, 2);
        
        return success;
    }
};
```

### Cheat Engine 스크립트 예제
```lua
-- Cheat Engine Lua 스크립트
[ENABLE]
aobscan(invincibility, 29 45 FC 8B 45 FC 85 C0)
alloc(newmem, 2048)

label(return)
label(originalcode)

newmem: // 무적 핵 구현
  mov [rbp-04], #1000  // HP를 항상 1000으로 설정
  mov eax, [rbp-04]
  jmp return

originalcode:
  sub [rbp-04], eax    // 원본 코드
  mov eax, [rbp-04]

invincibility+00:
  jmp newmem
  nop
  nop
return:

[DISABLE]
invincibility+00:
  db 29 45 FC 8B 45 FC  // 원본 바이트 복원

dealloc(newmem)
```

### 장점
- ✅ **범용성**: 모든 게임에 적용 가능
- ✅ **근본적 수정**: 게임 엔진 레벨에서 수정
- ✅ **제한 없음**: 개발사 API 제약 없음
- ✅ **학습 가치**: 저수준 시스템 이해 향상

### 단점
- ❌ **복잡성**: 어셈블리 및 시스템 지식 필요
- ❌ **위험성**: 게임 크래시 위험
- ❌ **업데이트 취약**: 게임 업데이트 시 패턴 변경
- ❌ **디버깅 어려움**: 오류 추적 및 해결 복잡

---

## 📊 접근법 비교표

| 특성 | 공식 지원 | 스크립트 기반 | 프레임워크 기반 | FromSoft 도구 | UETools | 바이너리 모딩 |
|------|----------|-------------|---------------|-------------|---------|-------------|
| **진입 장벽** | 낮음 | 중간 | 중간-높음 | 중간 | 낮음 | 높음 |
| **수정 범위** | 제한적 | 중간 | 광범위 | 매우 광범위 | 중간 | 무제한 |
| **안정성** | 높음 | 높음 | 중간 | 높음 | 높음 | 낮음 |
| **호환성** | 높음 | 중간 | 중간 | 중간 | 중간 | 낮음 |
| **학습 가치** | 낮음 | 중간 | 높음 | 높음 | 중간 | 매우 높음 |
| **범용성** | 낮음 | 낮음 | 중간 | 낮음 | 낮음 | 높음 |
| **설치 편의성** | 높음 | 중간 | 낮음 | 중간 | 매우 높음 | 중간 |
| **커뮤니티** | 중간 | 중간 | 높음 | 매우 높음 | 중간 | 높음 |

## 🎯 모딩 방식 선택 가이드

### 초급자 추천 경로
```
1. UETools (PAK 모딩) - 즉시 체험 가능 📦
   ↓
2. Steam Workshop (공식 지원 게임)
   ↓
3. WoW 애드온 or Skyrim 모딩
   ↓  
4. Minecraft Forge/Fabric
   ↓
5. Unity BepInEx 모딩
   ↓
6. 바이너리 모딩 (이 저장소) 🔧
```

### 목적별 추천

#### **게임 경험 개선이 목적**
- UETools (언리얼 엔진 게임 - 즉시 개선)
- FromSoft 도구 (Dark Souls/Elden Ring - 광범위한 수정)
- WoW 애드온 (UI 개선)
- Skyrim 모드 (콘텐츠 추가)
- Minecraft 모드 (새로운 아이템/블록)

#### **프로그래밍 학습이 목적**  
- Minecraft Forge (Java 학습)
- Unity BepInEx (C# 학습)
- FromSoft 도구 (파일 포맷 및 역공학 이해)
- UETools (콘솔 명령어 및 설정 파일)
- 바이너리 모딩 (어셈블리 학습)

#### **시스템 이해가 목적**
- 바이너리 모딩 (권장)
- FromSoft 도구 (커뮤니티 기반 도구 생태계)
- Unity BepInEx (중급)
- UETools (언리얼 엔진 구조)
- 리버스 엔지니어링 (고급)

## 🔗 추가 학습 자료

### 공식 문서
- [Steam Workshop Documentation](https://partner.steamgames.com/doc/features/workshop)
- [WoW Addon Development](https://warcraft.wiki.gg/wiki/World_of_Warcraft_API)
- [Skyrim Creation Kit](https://ck.uesp.net/wiki/Main_Page)
- [Minecraft Forge](https://docs.minecraftforge.net/)
- [Fabric Documentation](https://fabricmc.net/wiki/)
- [BepInEx Documentation](https://docs.bepinex.dev/)

### 커뮤니티
- **Reddit**: r/skyrimmods, r/feedthebeast, r/ModdingMC
- **Discord**: 각 게임별 모딩 커뮤니티
- **GitHub**: 오픈소스 모딩 프로젝트들
- **Nexus Mods**: 모드 공유 및 토론

---

**💡 핵심 통찰**: 

각 모딩 방식은 고유한 장단점을 가지고 있으며, 학습 목적과 목표하는 수정 범위에 따라 최적의 선택이 달라집니다. 이 저장소에서는 **바이너리 모딩**을 중심으로 학습하되, 다른 접근법들의 개념과 기법도 함께 이해함으로써 종합적인 모딩 지식을 습득할 수 있습니다.