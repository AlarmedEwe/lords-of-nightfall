#ifndef ENEMY_OBJECT_H
#define ENEMY_OBJECT_H

#include "../GameObject.h"
#include "../LONModel.h"
#include "raylib.h"
#include "raymath.h"
#include <string>

enum class EnemyState
{
    PatrolIdle,
    PatrolWalk,
    Chase,
    Attack,
    HitReact,
    Dead
};

struct PlayerObject; // Forward declaration

struct EnemyObject : public GameObject
{
    std::string name = "Inimigo";
    std::string category = "Big"; // Big, Blob, Flying
    std::string modelFile = "";

    int level = 1;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float attackPower = 10.0f;
    float defense = 2.0f;

    float xpReward = 30.0f;
    float goldReward = 15.0f;

    // AI & State Machine
    EnemyState state = EnemyState::PatrolIdle;
    float stateTimer = 0.0f;
    float attackCooldownTimer = 0.0f;
    float attackPauseDuration = 1.5f;
    float aggressionFactor = 0.5f; // 0.0 (pacifist/slow) to 1.0 (aggressive/fast attacks)

    Vector3 spawnPos = { 0.0f, 0.0f, 0.0f };
    Vector3 patrolTarget = { 0.0f, 0.0f, 0.0f };
    float patrolRadius = 12.0f;

    float walkSpeed = 3.0f;
    float chaseSpeed = 5.5f;
    float meshYaw = 0.0f;

    bool isProvoked = false;
    bool isDead = false;
    bool isMarkedForCleanup = false;
    float deathTimer = 0.0f;
    float hitReactTimer = 0.0f;

    LONModel monsterModel;
    Color tintColor = WHITE;

    EnemyObject(const std::string& monsterName, const std::string& cat, const std::string& lonModelFile, int lvl, Vector3 spawnPosition);

    void Init(Physics& physics) override;
    void Update(Physics& physics, float deltaTime) override;
    void UpdateAI(PlayerObject* player, float deltaTime);
    void Draw() override;
    void Shutdown(Physics& physics) override;

    void TakeDamage(float damage, int attackerLevel);
};

#endif // ENEMY_OBJECT_H
