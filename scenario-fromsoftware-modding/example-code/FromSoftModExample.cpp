/*
 * FromSoftware 모딩 예제
 *
 * 이 예제는 모드가 FromSoftware 게임의 내부 데이터 구조와 어떻게 상호작용하는지 시뮬레이션하며,
 * 특히 파라미터(param) 수정 및 개념적인 맵 데이터 조작에 중점을 둡니다.
 *
 * 주요 개념:
 * - 개념적인 파라미터 수정 (예: NpcParam, WeaponParam).
 * - 개념적인 맵 데이터 상호작용 (예: 적 속성 변경, 아이템 배치).
 * - 게임 데이터 구조 및 조작 시뮬레이션.
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

// --- 개념적 게임 데이터 구조 (시뮬레이션) ---

// 시뮬레이션된 NpcParam 구조체
struct NpcParam {
    int id;
    std::string name;
    int health;
    int stamina;
    int soulDrop;
    // ... 기타 NPC 속성
};

// 시뮬레이션된 WeaponParam 구조체
struct WeaponParam {
    int id;
    std::string name;
    int attackDamage;
    float scalingStrength;
    // ... 기타 무기 속성
};

// 시뮬레이션된 맵 엔티티 구조체
struct MapEntity {
    int entityId;
    std::string type;
    float posX, posY, posZ;
    // ... 기타 엔티티 속성
};

// 시뮬레이션된 게임 데이터 테이블
std::map<int, NpcParam> NpcParams;
std::map<int, WeaponParam> WeaponParams;
std::map<int, MapEntity> MapEntities;

// --- 시뮬레이션된 게임 엔진 함수 ---

// 더미 게임 데이터 초기화
void InitializeGameData() {
    NpcParams[10010100] = {10010100, "기본 병사", 100, 50, 25};
    NpcParams[10020200] = {10020200, "엘리트 기사", 500, 200, 100};
    
    WeaponParams[100] = {100, "롱소드", 100, 0.5f};
    WeaponParams[200] = {200, "대검", 250, 0.8f};
    
    MapEntities[1] = {1, "적", 10.0f, 0.0f, 15.0f};
    MapEntities[2] = {2, "아이템", 20.0f, 0.0f, 25.0f};
    MapEntities[3] = {3, "보스", 50.0f, 0.0f, 50.0f};
    
    std::cout << "[게임 엔진] 더미 게임 데이터 초기화됨." << std::endl;
}

// --- 모딩 함수 (개념적) ---

// NPC 체력 수정
void ModifyNpcHealth(int npcId, int newHealth) {
    auto it = NpcParams.find(npcId);
    if (it != NpcParams.end()) {
        std::cout << "[모드] NPC 수정 중: " << it->second.name << " (ID: " << npcId << ") 체력: " << it->second.health << " -> " << newHealth << std::endl;
        it->second.health = newHealth;
    } else {
        std::cout << "[모드] ID " << npcId << "를 가진 NPC를 찾을 수 없습니다." << std::endl;
    }
}

// 무기 공격력 수정
void ModifyWeaponDamage(int weaponId, int newDamage) {
    auto it = WeaponParams.find(weaponId);
    if (it != WeaponParams.end()) {
        std::cout << "[모드] 무기 수정 중: " << it->second.name << " (ID: " << weaponId << ") 공격력: " << it->second.attackDamage << " -> " << newDamage << std::endl;
        it->second.attackDamage = newDamage;
    } else {
        std::cout << "[모드] ID " << weaponId << "를 가진 무기를 찾을 수 없습니다." << std::endl;
    }
}

// 맵 엔티티 이동
void MoveMapEntity(int entityId, float x, float y, float z) {
    auto it = MapEntities.find(entityId);
    if (it != MapEntities.end()) {
        std::cout << "[모드] 엔티티 이동 중: " << it->second.type << " (ID: " << entityId << ") (" << it->second.posX << ", " << it->second.posY << ", " << it->second.posZ << ")에서 (" << x << ", " << y << ", " << z << ")로" << std::endl;
        it->second.posX = x;
        it->second.posY = y;
        it->second.posZ = z;
    } else {
        std::cout << "[모드] ID " << entityId << "를 가진 엔티티를 찾을 수 없습니다." << std::endl;
    }
}

// --- 메인 함수 (데모/테스트용) ---
int main() {
    std::cout << "=== FromSoftware 모딩 예제 시뮬레이션 ===" << std::endl;
    
    // 게임 데이터 초기화 시뮬레이션
    InitializeGameData();
    
    std::cout << "\n--- 모딩 작업 ---" << std::endl;
    
    // 모드 작업 1: 기본 병사를 더 강하게 만들기
    ModifyNpcHealth(10010100, 200); // 체력 두 배
    
    // 모드 작업 2: 롱소드 버프
    ModifyWeaponDamage(100, 150); // 공격력 증가
    
    // 모드 작업 3: 적을 새 위치로 이동
    MoveMapEntity(1, 50.0f, 10.0f, 5.0f);
    
    std::cout << "\n--- 현재 게임 데이터 상태 (모딩 후) ---" << std::endl;
    std::cout << "기본 병사 체력: " << NpcParams[10010100].health << std::endl;
    std::cout << "롱소드 공격력: " << WeaponParams[100].attackDamage << std::endl;
    std::cout << "엔티티 1 위치: (" << MapEntities[1].posX << ", " << MapEntities[1].posY << ", " << MapEntities[1].posZ << ")" << std::endl;
    
    std::cout << "\n시뮬레이션 완료. Enter를 눌러 종료하세요.";
    std::cin.ignore();
    std::cin.get();
    
    return 0;
}
