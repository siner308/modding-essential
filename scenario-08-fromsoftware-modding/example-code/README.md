# FromSoftware 모딩 예제 코드

이 디렉터리에는 FromSoftware 게임(예: 엘든 링, 다크 소울)을 위한 개념적인 모딩 기술을 보여주는 예제 코드가 포함되어 있습니다.

## FromSoftModExample.cpp

이 예제는 모드가 FromSoftware 게임의 내부 데이터 구조와 어떻게 상호작용하는지 시뮬레이션하며, 특히 파라미터(param) 수정 및 개념적인 맵 데이터 조작에 중점을 둡니다.

### 주요 개념:

- **파라미터(Param) 수정**: 게임 파라미터 테이블(예: `NpcParam`, `WeaponParam`)의 값을 읽고 쓰는 것을 시뮬레이션합니다.
- **맵 데이터 상호작용**: 맵 엔티티(예: 적 속성 변경, 아이템 배치)와의 개념적인 상호작용을 보여줍니다.
- **게임 데이터 구조**: 동작을 변경하기 위해 게임 메모리 내의 특정 오프셋에 접근하는 아이디어를 강조합니다.

### 사용법 (개념적):

1.  이 코드는 시뮬레이션이며 실행 중인 게임을 직접 수정하지 않습니다.
2.  DSMapStudio 또는 Smithbox와 같은 도구를 사용하여 모드가 수행할 작업 유형을 보여줍니다.
3.  `main` 함수는 모드가 개념적으로 게임 데이터를 어떻게 변경하는지 보여줍니다.

```cpp
// 개념적인 파라미터 수정 예제
// (실제 구현은 특정 오프셋에서의 메모리 읽기/쓰기를 포함합니다)
struct NpcParam {
    int health;
    int stamina;
    int soulDrop;
    // ... 기타 NPC 속성
};

void ModifyNpcParam(int npcId, int newHealth) {
    // 메모리에서 NPC 파라미터 데이터를 찾는 것을 시뮬레이션합니다.
    NpcParam* npc = GetNpcParamById(npcId);
    if (npc) {
        npc->health = newHealth;
        std::cout << "[Mod] NPC " << npcId << " 체력: " << npc->health << " -> " << newHealth << std::endl;
    }
}

// 개념적인 맵 엔티티 수정 예제
struct MapEntity {
    int entityId;
    int type;
    float posX, posY, posZ;
    // ... 기타 엔티티 속성
};

void ChangeMapEntityPosition(int entityId, float newX, float newY, float newZ) {
    // 메모리에서 맵 엔티티 데이터를 찾는 것을 시뮬레이션합니다.
    MapEntity* entity = GetMapEntityById(entityId);
    if (entity) {
        entity->posX = newX;
        entity->posY = newY;
        entity->posZ = newZ;
        std::cout << "[Mod] 엔티티 " << entityId << " 위치: (" << newX << ", " << newY << ", " << newZ << ")로 이동" << std::endl;
    }
}
```
