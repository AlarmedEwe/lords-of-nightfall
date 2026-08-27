#include "EnemyObject.h"
#include "PlayerObject.h"
#include "TerrainObject.h"
#include "EnemyAI.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

EnemyObject::EnemyObject(const std::string& monsterName, const std::string& cat, const std::string& lonModelFile, int lvl, Vector3 spawnPosition)
    : name(monsterName), category(cat), modelFile(lonModelFile)
{
    level = lvl;
    position = spawnPosition;
    spawnPos = spawnPosition;
    patrolTarget = spawnPosition;

    // Escala atributos de acordo com o nível
    maxHealth = 40.0f + level * 20.0f;
    health = maxHealth;
    attackPower = 8.0f + level * 3.5f;
    defense = 1.0f + level * 1.2f;

    xpReward = 25.0f + level * 15.0f;
    goldReward = 10.0f + level * 8.0f;

    // Fator aleatório de agressividade (0.15 a 0.95)
    aggressionFactor = 0.15f + (float)(rand() % 80) / 100.0f;
    attackPauseDuration = 2.0f - (aggressionFactor * 1.3f);

    if (category == "Blob")
    {
        walkSpeed = 2.5f;
        chaseSpeed = 4.5f;
        tintColor = WHITE;
    }
    else if (category == "Flying")
    {
        walkSpeed = 3.5f;
        chaseSpeed = 6.5f;
        tintColor = WHITE;
    }
    else
    {
        walkSpeed = 3.0f;
        chaseSpeed = 5.5f;
        tintColor = WHITE;
    }
}

void EnemyObject::Init(Physics& physics)
{
    (void)physics;
    monsterModel.Load(modelFile.c_str(), "Atlas_Monsters.png");

    if (TerrainObject::s_Instance)
    {
        float terrainY = TerrainObject::s_Instance->GetHeightAt(position.x, position.z);
        if (category == "Flying")
        {
            position.y = terrainY + 2.8f;
        }
        else
        {
            position.y = terrainY;
        }
        spawnPos.y = position.y;
    }
}

void EnemyObject::Update(Physics& physics, float deltaTime)
{
    (void)physics;
    if (hitFlashTimer > 0.0f)
    {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer < 0.0f) hitFlashTimer = 0.0f;
    }

    if (TerrainObject::s_Instance)
    {
        float terrainY = TerrainObject::s_Instance->GetHeightAt(position.x, position.z);
        if (category == "Flying")
        {
            float bobbing = sinf((float)GetTime() * 2.2f + (position.x * 0.5f + position.z * 0.5f)) * 0.35f;
            float targetY = terrainY + 2.8f + bobbing;
            position.y = targetY;
            if (position.y < terrainY + 1.0f) position.y = terrainY + 1.0f;
        }
        else
        {
            position.y = terrainY;
        }
    }

    if (monsterModel.isLoaded)
    {
        monsterModel.Update(deltaTime);
    }

    if (isDead)
    {
        deathTimer += deltaTime;
        if (deathTimer >= 2.0f)
        {
            isMarkedForCleanup = true;
        }
    }
}

void EnemyObject::UpdateAI(PlayerObject* player, float deltaTime, DamageFeedbackSystem* feedback)
{
    EnemyAI::Update(*this, player, deltaTime, feedback);
}

void EnemyObject::Draw()
{
    if (monsterModel.isLoaded && !isMarkedForCleanup)
    {
        Color drawTint = (hitFlashTimer > 0.0f) ? (Color){ 255, 60, 60, 255 } : tintColor;
        monsterModel.Draw(position, meshYaw, 1.0f, drawTint);
    }
}

void EnemyObject::Shutdown(Physics& physics)
{
    (void)physics;
    monsterModel.Unload();
}

void EnemyObject::TakeDamage(float damage, int attackerLevel)
{
    (void)attackerLevel;
    if (isDead) return;

    health -= damage;
    isProvoked = true;
    hitFlashTimer = 0.25f;

    if (health <= 0.0f)
    {
        health = 0.0f;
        isDead = true;
        state = EnemyState::Dead;
        if (monsterModel.isLoaded) monsterModel.SetAnimation(5); // Death
    }
    else
    {
        state = EnemyState::HitReact;
        hitReactTimer = 0.4f;
        if (monsterModel.isLoaded) monsterModel.SetAnimation(4); // HitReact
    }
}
