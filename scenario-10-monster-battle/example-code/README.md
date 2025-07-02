# 몬스터 배틀 예제 코드

이 디렉터리에는 AI 상호작용 및 시뮬레이션된 전투에 초점을 맞춘 게임 내 몬스터 배틀 시스템의 개념적인 구현을 보여주는 예제 코드가 포함되어 있습니다.

## MonsterBattleExample.cpp

이 예제는 다양한 유형의 "몬스터"(간단한 AI 로직으로 표현됨)가 서로 싸울 수 있는 단순화된 전투 아레나를 시뮬레이션합니다. 다음 개념을 강조합니다:

- **진영 시스템**: 전투원을 결정하기 위해 엔티티가 팀(예: 아군, 적대)으로 분류되는 방법.
- **기본 AI 동작**: 대상을 공격하기 위한 간단한 의사 결정.
- **전투 시뮬레이션**: 턴 기반 또는 단순화된 실시간 전투 메커니즘.
- **이벤트 처리**: 이벤트(예: 공격, 사망)를 처리하는 방법.

### 주요 개념:

- **엔티티 정의**: 체력, 공격력, 진영을 가진 전투원 표현.
- **대상 선택**: 적을 선택하는 로직.
- **공격 메커니즘**: 피해 계산 시뮬레이션.
- **전투 루프**: 승자가 결정될 때까지 전투 흐름 관리.

### 사용법 (개념적):

1.  이 코드는 시뮬레이션이며 실제 게임 엔진과 직접 상호작용하지 않습니다.
2.  게임에서 몬스터 배틀 기능을 구동하는 기본 로직을 보여줍니다.
3.  `main` 함수는 전투 시나리오를 설정하고 시뮬레이션을 실행합니다.

```cpp
// 단순화된 전투원 엔티티 예제
struct Combatant {
    std::string name;
    int health;
    int attack;
    int faction;
    bool isAlive() const { return health > 0; }
};

// 전투 시뮬레이션 함수 예제
void SimulateBattle(Combatant& c1, Combatant& c2) {
    std::cout << c1.name << " (진영 " << c1.faction << ") vs " << c2.name << " (진영 " << c2.faction << ")\n";
    while (c1.isAlive() && c2.isAlive()) {
        // c1이 c2 공격
        int damage1 = std::max(0, c1.attack - (c2.health / 10)); // 간단한 방어
        c2.health -= damage1;
        std::cout << c1.name << "이(가) " << c2.name << "을(를) 공격하여 " << damage1 << " 피해를 입혔습니다. " << c2.name << " 체력: " << c2.health << "\n";

        if (!c2.isAlive()) break;

        // c2가 c1 공격
        int damage2 = std::max(0, c2.attack - (c1.health / 10));
        c1.health -= damage2;
        std::cout << c2.name << "이(가) " << c1.name << "을(를) 공격하여 " << damage2 << " 피해를 입혔습니다. " << c1.name << " 체력: " << c1.health << "\n";
    }
    if (c1.isAlive()) std::cout << c1.name << " 승리!\n";
    else std::cout << c2.name << " 승리!\n";
}

// 사용 예제
Combatant monsterA = {"고블린", 100, 10, 1};
Combatant monsterB = {"오크", 150, 15, 2};
SimulateBattle(monsterA, monsterB);
```
