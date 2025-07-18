# 🟡 몬스터 대전 시스템 구축

**난이도**: 중급-고급 | **학습 시간**: 3-4주 | **접근법**: 멀티 플랫폼 (FromSoft + 바이너리)

## 🎯 학습 목표

다양한 게임에서 몬스터나 보스를 소환하고 서로 싸우게 만드는 AI 대전 시스템을 구축합니다.

### 핵심 학습 포인트
- ✅ AI 행동 패턴 분석 및 수정
- ✅ 팩션(진영) 시스템 조작
- ✅ 스폰 시스템 이해와 활용
- ✅ 대전 밸런싱 및 스크립팅
- ✅ 크로스 게임 호환 기법

## 📚 이론 학습: AI 대전의 핵심 개념

### 게임 AI 시스템 구조
```
AI 시스템 계층 구조:
├── 1. Faction System (진영/적대관계)
├── 2. Think Parameter (행동 패턴)
├── 3. Target Selection (대상 선택)
├── 4. Combat State (전투 상태)
└── 5. Event Triggers (이벤트 트리거)
```

### 대전 유형

#### **1. 보스 vs 보스**
```
예시: 말레니아 vs 라다곤 (Elden Ring)
- 두 보스의 팩션을 서로 적대로 설정
- 동일 맵에 스폰
- 플레이어는 관전자 모드
```

#### **2. 몬스터 군단전**
```
예시: 기사 vs 오크 군단 (Dark Souls)
- 다수 vs 다수 대규모 전투
- 팩션별 리스폰 시스템
- 승리 조건 설정
```

#### **3. 진화형 토너먼트**
```
예시: 몬스터 토너먼트 (다양한 게임)
- 라운드별 승부
- 승자는 체력 회복 후 다음 상대
- 최종 우승자 결정
```

## 🎮 게임별 구현 방법

### FromSoftware 게임 (Elden Ring/Dark Souls)

#### **1. 팩션 시스템 조작**

```bash
# Smithbox - NpcParam 편집
1. NpcParam 열기
2. 대상 몬스터들 찾기
3. TeamType 값 변경:
   - 0: 중립
   - 1: 플레이어 적
   - 2: 플레이어 아군
   - 3: 커스텀 팩션 A
   - 4: 커스텀 팩션 B
```

#### **2. AI 행동 패턴 수정**

```python
# ThinkParam 편집 (의사코드)
class MonsterAI:
    def __init__(self, monster_id):
        self.target_priority = {
            'player': 10,
            'enemy_faction': 8,  # 새로 추가
            'neutral': 5
        }
    
    def select_target(self):
        # 가장 가까운 적대 팩션 우선 타겟
        enemies = find_nearby_enemies()
        return closest_enemy_by_faction()
```

#### **3. 스폰 및 이벤트 스크립팅**

```lua
-- EMEVD 이벤트 예제
function MonsterBattleEvent()
    -- 전투 구역 설정
    local arena_center = {X=100, Y=0, Z=200}
    local arena_radius = 50
    
    -- 팩션 A 스폰 (말레니아)
    spawn_monster(boss_malenia, arena_center.X-20, arena_center.Y, arena_center.Z)
    set_faction(boss_malenia, FACTION_A)
    
    -- 팩션 B 스폰 (라다곤)
    spawn_monster(boss_radagon, arena_center.X+20, arena_center.Y, arena_center.Z)
    set_faction(boss_radagon, FACTION_B)
    
    -- 플레이어 관전 모드
    set_player_invincible(true)
    teleport_player(arena_center.X, arena_center.Y+30, arena_center.Z)
    
    -- 승부 판정
    while true do
        if is_dead(boss_malenia) then
            display_message("라다곤 승리!")
            break
        elseif is_dead(boss_radagon) then
            display_message("말레니아 승리!")
            break
        end
        wait(1.0)
    end
end
```

### 언리얼 엔진 게임 (UE4SS 활용)

#### **1. Lua 스크립트 기반 AI 조작**

```lua
-- UE4SS Lua 모드
local MonsterBattle = {}

function MonsterBattle:init()
    print("Monster Battle System Loading...")
    
    -- F5키로 대전 시작
    RegisterKeyBind(Key.F5, function()
        self:StartBattle()
    end)
end

function MonsterBattle:StartBattle()
    -- 모든 적을 찾기
    local enemies = FindAllOf("Pawn")
    
    -- 두 그룹으로 나누기
    local team_a = {}
    local team_b = {}
    
    for i, enemy in ipairs(enemies) do
        if i % 2 == 0 then
            table.insert(team_a, enemy)
            self:SetTeam(enemy, "TeamA")
        else
            table.insert(team_b, enemy)
            self:SetTeam(enemy, "TeamB")
        end
    end
    
    print("Battle Started: " .. #team_a .. " vs " .. #team_b)
end

function MonsterBattle:SetTeam(actor, team_name)
    -- AI Controller 찾기
    local ai_controller = actor:GetController()
    if ai_controller:IsValid() then
        -- 팀 설정 (게임마다 다름)
        ai_controller:SetGenericTeamId(team_name)
    end
end
```

### Unity 게임 (BepInEx + Harmony)

#### **1. AI 컴포넌트 패치**

```csharp
[BepInPlugin("MonsterBattle", "Monster Battle System", "1.0")]
public class MonsterBattlePlugin : BaseUnityPlugin
{
    void Awake()
    {
        var harmony = new Harmony("monster.battle.patch");
        harmony.PatchAll();
    }
}

[HarmonyPatch(typeof(EnemyAI), "ChooseTarget")]
public class EnemyTargetPatch
{
    static void Postfix(EnemyAI __instance, ref GameObject __result)
    {
        // 기존 타겟 선택 로직 대신 팩션 기반 선택
        var newTarget = FindNearestEnemyFaction(__instance);
        if (newTarget != null)
        {
            __result = newTarget;
        }
    }
    
    static GameObject FindNearestEnemyFaction(EnemyAI ai)
    {
        var myFaction = ai.GetComponent<FactionComponent>()?.faction;
        var enemies = FindObjectsOfType<EnemyAI>();
        
        GameObject closest = null;
        float closestDistance = float.MaxValue;
        
        foreach (var enemy in enemies)
        {
            var enemyFaction = enemy.GetComponent<FactionComponent>()?.faction;
            if (enemyFaction != myFaction)
            {
                float distance = Vector3.Distance(ai.transform.position, enemy.transform.position);
                if (distance < closestDistance)
                {
                    closest = enemy.gameObject;
                    closestDistance = distance;
                }
            }
        }
        
        return closest;
    }
}

// 팩션 컴포넌트
public class FactionComponent : MonoBehaviour
{
    public enum Faction { Neutral, TeamA, TeamB, TeamC }
    public Faction faction = Faction.Neutral;
}
```

## 🔧 고급 대전 시스템 구현

### 1. 다단계 토너먼트 시스템

```python
# 토너먼트 매니저 (의사코드)
class TournamentManager:
    def __init__(self):
        self.participants = []
        self.current_round = 1
        self.bracket = []
    
    def add_monster(self, monster_id, stats):
        self.participants.append({
            'id': monster_id,
            'hp': stats.hp,
            'attack': stats.attack,
            'wins': 0
        })
    
    def generate_bracket(self):
        import random
        random.shuffle(self.participants)
        
        # 1회전 대진표 생성
        self.bracket = []
        for i in range(0, len(self.participants), 2):
            match = (self.participants[i], self.participants[i+1])
            self.bracket.append(match)
    
    def fight(self, monster_a, monster_b):
        # 실제 게임에서 대전 실행
        spawn_monsters_in_arena(monster_a['id'], monster_b['id'])
        
        # 승부 대기
        winner = wait_for_battle_end()
        
        # 승자 기록
        winner['wins'] += 1
        return winner
    
    def run_tournament(self):
        while len(self.participants) > 1:
            next_round = []
            
            for match in self.bracket:
                winner = self.fight(match[0], match[1])
                next_round.append(winner)
            
            self.participants = next_round
            self.current_round += 1
            self.generate_bracket()
        
        champion = self.participants[0]
        print(f"Tournament Champion: {champion['id']}")
```

### 2. 실시간 밸런싱 시스템

```csharp
// 동적 밸런싱 시스템
public class DynamicBalancer : MonoBehaviour
{
    private Dictionary<string, MonsterStats> baseStats = new();
    private Dictionary<string, float> winRates = new();
    
    void Start()
    {
        // 기본 스탯 로드
        LoadBaseStats();
        
        // 주기적으로 밸런싱 조정
        InvokeRepeating(nameof(AdjustBalance), 10f, 30f);
    }
    
    void AdjustBalance()
    {
        foreach (var monster in winRates.Keys)
        {
            float winRate = winRates[monster];
            
            // 승률이 너무 높으면 너프
            if (winRate > 0.7f)
            {
                AdjustMonsterStats(monster, -0.1f);
                Debug.Log($"{monster} nerfed due to {winRate:P} win rate");
            }
            // 승률이 너무 낮으면 버프
            else if (winRate < 0.3f)
            {
                AdjustMonsterStats(monster, 0.1f);
                Debug.Log($"{monster} buffed due to {winRate:P} win rate");
            }
        }
    }
    
    void AdjustMonsterStats(string monsterId, float multiplier)
    {
        var monsters = FindObjectsOfType<MonsterAI>();
        foreach (var monster in monsters)
        {
            if (monster.monsterId == monsterId)
            {
                monster.health *= (1 + multiplier);
                monster.damage *= (1 + multiplier);
            }
        }
    }
}
```

### 3. 스펙테이터 모드 구현

```lua
-- 관전자 모드 (UE4SS Lua)
local SpectatorMode = {}

function SpectatorMode:init()
    self.camera_targets = {}
    self.current_target_index = 1
    self.is_spectating = false
    
    -- 관전 모드 토글
    RegisterKeyBind(Key.F6, function()
        self:ToggleSpectatorMode()
    end)
    
    -- 카메라 타겟 변경
    RegisterKeyBind(Key.TAB, function()
        if self.is_spectating then
            self:NextTarget()
        end
    end)
end

function SpectatorMode:ToggleSpectatorMode()
    self.is_spectating = not self.is_spectating
    
    if self.is_spectating then
        -- 플레이어 무적/투명 모드
        local player = GetPlayer()
        player:SetInvincible(true)
        player:SetVisibility(false)
        
        -- 전투 중인 몬스터들 찾기
        self:FindCombatTargets()
        
        print("Spectator Mode: ON")
    else
        -- 정상 모드 복원
        local player = GetPlayer()
        player:SetInvincible(false)
        player:SetVisibility(true)
        
        print("Spectator Mode: OFF")
    end
end

function SpectatorMode:FindCombatTargets()
    self.camera_targets = {}
    
    local enemies = FindAllOf("Enemy")
    for _, enemy in ipairs(enemies) do
        if enemy:IsInCombat() then
            table.insert(self.camera_targets, enemy)
        end
    end
end

function SpectatorMode:NextTarget()
    if #self.camera_targets == 0 then return end
    
    self.current_target_index = self.current_target_index + 1
    if self.current_target_index > #self.camera_targets then
        self.current_target_index = 1
    end
    
    local target = self.camera_targets[self.current_target_index]
    self:SetCameraTarget(target)
end

function SpectatorMode:SetCameraTarget(target)
    local camera = GetPlayerCamera()
    if camera:IsValid() and target:IsValid() then
        -- 타겟 주변 회전 카메라
        camera:SetFollowTarget(target)
        camera:SetDistance(500) -- 5미터 거리
        camera:SetHeight(200)   -- 2미터 높이
    end
end
```

## 📋 실전 프로젝트: 몬스터 배틀 로얄

### 프로젝트 목표
```
"Elden Ring 보스들의 배틀 로얄"
- 참가자: 말레니아, 라다곤, 모그, 마리케스, 플라시독스
- 방식: 토너먼트 대진표
- 특징: 실시간 관전, 승부 예측, 통계 기록
```

### 구현 단계

#### **1단계: 환경 준비 (1주)**
```bash
필요 도구:
- Smithbox (최신 버전)
- Mod Engine 2
- UE4SS (선택사항)
- OBS Studio (녹화용)

기본 설정:
1. 전용 아레나 맵 선택/제작
2. 보스 소환 위치 마킹
3. 관전 포인트 설정
```

#### **2단계: AI 시스템 수정 (1주)**
```bash
NpcParam 수정:
1. 각 보스의 TeamType 변경
2. 상호 적대 관계 설정
3. 어그로 범위 조정
4. 체력/공격력 밸런싱

ThinkParam 분석:
1. 각 보스의 AI 패턴 연구
2. 플레이어 의존적 행동 제거
3. 대 몬스터 전투 최적화
```

#### **3단계: 이벤트 스크립팅 (1주)**
```bash
EMEVD 이벤트 작성:
1. 보스 순차 소환 시스템
2. 사망 감지 및 승부 판정
3. 다음 라운드 자동 진행
4. 결과 메시지 출력

자동화 시스템:
1. 토너먼트 브래킷 관리
2. 승자 체력 회복
3. 패자 시체 제거
4. 통계 데이터 기록
```

#### **4단계: 고급 기능 (1주)**
```bash
관전 시스템:
1. 플레이어 투명/무적 모드
2. 자유 카메라 구현
3. 슬로우 모션 효과
4. 인스턴트 리플레이

데이터 분석:
1. 승부 결과 로그
2. 데미지 통계 수집
3. 승률 분석
4. 밸런스 패치 제안
```

## 🎥 컨텐츠 제작 아이디어

### 1. 몬스터 vs 몬스터 시리즈
```
"다크 소울 보스 최강자는?"
- 각 게임별 보스 토너먼트
- 시청자 투표로 예상 승자 맞추기
- 승부 하이라이트 편집
```

### 2. 크로스 게임 매치업
```
"엘든 링 vs 세키로 보스전"
- 서로 다른 게임의 보스들 대결
- 스탯 노멀라이즈로 공정한 경쟁
- 팬 투표로 드림 매치 선정
```

### 3. 커뮤니티 이벤트
```
"내가 만든 커스텀 보스"
- 사용자 제작 보스 공모
- 기존 보스와 대결
- 최고의 커스텀 보스 선정
```

## 🔗 확장 가능성

### 다른 게임 지원
- **Monster Hunter**: 몬스터 간 생태계 전투
- **Pokémon**: AI 트레이너 배틀 시뮬레이션  
- **Skyrim**: 드래곤 vs 거인 vs 마법사
- **Witcher 3**: 몬스터 생태계 시뮬레이션

### 기술적 발전
- **머신러닝**: AI 학습으로 더 나은 전투 패턴
- **실시간 스트리밍**: 라이브 토너먼트 중계
- **VR 지원**: 가상현실 관전 모드
- **블록체인**: NFT 기반 몬스터 소유권

## 💡 학습 성과

이 과정를 완료하면:
- 🧠 **AI 시스템 이해**: 게임 AI의 작동 원리 완전 파악
- 🎮 **게임 밸런싱**: 실제 게임 개발자 수준의 밸런싱 감각
- 🎬 **컨텐츠 제작**: 유튜브/트위치 스트리밍 콘텐츠 제작 능력
- 🔧 **시스템 설계**: 복잡한 이벤트 시스템 설계 및 구현

---

**이전 학습**: [PAK 로더 구현](../scenario-pak-loader/)