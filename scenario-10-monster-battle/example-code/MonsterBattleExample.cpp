/*
 * 몬스터 배틀 예제
 *
 * 이 예제는 다양한 종류의 "몬스터"(간단한 AI 로직으로 표현됨)가 서로 싸울 수 있는
 * 간소화된 전투 아레나를 시뮬레이션합니다. 다음 개념들을 강조합니다:
 * - 진영 시스템: 전투원을 결정하기 위해 엔티티가 팀(예: 아군, 적군)으로 분류되는 방식.
 * - 기본 AI 행동: 대상을 공격하기 위한 간단한 의사 결정.
 * - 전투 시뮬레이션: 턴 기반 또는 간소화된 실시간 전투 메커니즘.
 * - 이벤트 처리: 공격, 사망과 같은 이벤트가 처리될 수 있는 방식.
 *
 * 시연된 주요 개념:
 * - 엔티티 정의: 체력, 공격력, 진영을 가진 전투원 표현.
 * - 대상 선택: 상대방을 선택하기 위한 로직.
 * - 공격 메커니즘: 피해 계산 시뮬레이션.
 * - 전투 루프: 승자가 결정될 때까지 전투 흐름 관리.
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>

// --- 시뮬레이션된 게임 데이터 구조 ---

// 전투의 전투원을 나타냅니다.
struct Combatant {
    std::string name; // 이름
    int health;       // 현재 체력
    int maxHealth;    // 최대 체력
    int attack;       // 공격력
    int defense;      // 방어력
    int factionId;    // 진영 ID (1 = 플레이어, 2 = 몬스터 A, 3 = 몬스터 B 등)
    bool isAlive = true; // 생존 여부

    Combatant(std::string n, int hp, int atk, int def, int faction) 
        : name(n), health(hp), maxHealth(hp), attack(atk), defense(def), factionId(faction) {}

    void TakeDamage(int damage) {
        health -= damage;
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
    }

    // 체력을 회복합니다.
    void Heal(int amount) {
        health += amount;
        if (health > maxHealth) {
            health = maxHealth;
        }
    }

    // 다른 전투원이 적대적인지 확인합니다.
    bool IsHostile(const Combatant& other) const {
        // 간단한 적대성: 다른 진영은 적대적입니다.
        return factionId != other.factionId;
    };
};

// --- 전투 시뮬레이션 로직 ---

// 전역 난수 생성기
std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

// 전투의 한 턴을 시뮬레이션합니다.
void SimulateTurn(Combatant& attacker, Combatant& defender) {
    if (!attacker.isAlive || !defender.isAlive) return;

    // 피해 계산 (간단한 공식)
    int damage = std::max(0, attacker.attack - defender.defense);
    defender.TakeDamage(damage);

    std::cout << attacker.name <<