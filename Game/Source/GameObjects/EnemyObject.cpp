#include "EnemyObject.h"
#include "PlayerObject.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

EnemyObject::EnemyObject(const std::string& monsterName, const std::string& cat, const std::string& lonModelFile, int lvl, Vector3 spawnPosition)
    : name(monsterName), category(cat), modelFile(lonModelFile), level(lvl)
{
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
    attackPauseDuration = 2.0f - (aggressionFactor * 1.3f); // 0.7s até 2.0s de pausa

    if (category == "Blob")
    {
        walkSpeed = 2.5f;
        chaseSpeed = 4.5f;
        tintColor = (Color){ 210, 255, 210, 255 };
    }
    else if (category == "Flying")
    {
        walkSpeed = 3.5f;
        chaseSpeed = 6.5f;
        tintColor = (Color){ 220, 220, 255, 255 };
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
}

void EnemyObject::Update(Physics& physics, float deltaTime)
{
    (void)physics;
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

void EnemyObject::UpdateAI(PlayerObject* player, float deltaTime)
{
    if (isDead || player == nullptr) return;

    float distToPlayer = Vector3Distance(position, player->position);

    // Regra dos 5 níveis: se o jogador for 5+ níveis acima e não provocado, não ataca por padrão
    bool ignoresPlayer = (player->level >= level + 5) && !isProvoked;

    // Se o jogador morreu, reseta provocação e volta para patrulha
    if (player->health <= 0.0f)
    {
        isProvoked = false;
        state = EnemyState::PatrolIdle;
        stateTimer = 1.0f;
    }

    switch (state)
    {
    case EnemyState::HitReact:
    {
        hitReactTimer -= deltaTime;
        if (hitReactTimer <= 0.0f)
        {
            state = EnemyState::Chase;
        }
        break;
    }

    case EnemyState::PatrolIdle:
    {
        if (monsterModel.isLoaded) monsterModel.SetAnimation(0); // Idle

        stateTimer -= deltaTime;

        if (!ignoresPlayer && distToPlayer <= 12.0f)
        {
            state = EnemyState::Chase;
        }
        else if (stateTimer <= 0.0f)
        {
            // Escolhe ponto de patrulha aleatório
            float angle = ((float)(rand() % 360)) * DEG2RAD;
            float dist = 3.0f + (float)(rand() % (int)(patrolRadius * 10)) / 10.0f;

            patrolTarget = {
                spawnPos.x + cosf(angle) * dist,
                spawnPos.y,
                spawnPos.z + sinf(angle) * dist
            };

            state = EnemyState::PatrolWalk;
            stateTimer = 8.0f; // Timeout limite para patrulhar
        }
        break;
    }

    case EnemyState::PatrolWalk:
    {
        if (monsterModel.isLoaded) monsterModel.SetAnimation(1); // Walk

        stateTimer -= deltaTime;

        if (!ignoresPlayer && distToPlayer <= 12.0f)
        {
            state = EnemyState::Chase;
        }
        else
        {
            Vector3 diff = { patrolTarget.x - position.x, 0.0f, patrolTarget.z - position.z };
            float distToTarget = Vector3Length(diff);

            if (distToTarget < 0.6f || stateTimer <= 0.0f)
            {
                state = EnemyState::PatrolIdle;
                stateTimer = 1.5f + (float)(rand() % 20) / 10.0f;
            }
            else
            {
                Vector3 dir = Vector3Normalize(diff);
                position.x += dir.x * walkSpeed * deltaTime;
                position.z += dir.z * walkSpeed * deltaTime;

                meshYaw = atan2f(dir.x, dir.z) * RAD2DEG;
            }
        }
        break;
    }

    case EnemyState::Chase:
    {
        if (monsterModel.isLoaded) monsterModel.SetAnimation(2); // Run

        if (distToPlayer > 22.0f)
        {
            // Perdeu o jogador de vista
            state = EnemyState::PatrolIdle;
            stateTimer = 1.5f;
        }
        else if (distToPlayer <= 2.2f)
        {
            state = EnemyState::Attack;
            stateTimer = 0.0f;
            attackCooldownTimer = 0.0f;
        }
        else
        {
            Vector3 diff = { player->position.x - position.x, 0.0f, player->position.z - position.z };
            Vector3 dir = Vector3Normalize(diff);

            position.x += dir.x * chaseSpeed * deltaTime;
            position.z += dir.z * chaseSpeed * deltaTime;

            meshYaw = atan2f(dir.x, dir.z) * RAD2DEG;
        }
        break;
    }

    case EnemyState::Attack:
    {
        if (monsterModel.isLoaded) monsterModel.SetAnimation(3); // Attack

        stateTimer += deltaTime;
        attackCooldownTimer += deltaTime;

        // Desfere o golpe aos 0.3s de animação
        if (stateTimer >= 0.3f && stateTimer - deltaTime < 0.3f)
        {
            if (distToPlayer <= 2.6f && player->health > 0.0f)
            {
                float damage = std::max(1.0f, attackPower - player->GetTotalDefense());
                player->health -= damage;
                if (player->health < 0.0f) player->health = 0.0f;
            }
        }

        // Final do ciclo de ataque + pausa baseada em agressividade
        if (stateTimer >= (0.6f + attackPauseDuration))
        {
            if (distToPlayer <= 2.2f)
            {
                stateTimer = 0.0f; // Ataca novamente
            }
            else
            {
                state = EnemyState::Chase;
            }
        }
        break;
    }

    case EnemyState::Dead:
        break;
    }
}

void EnemyObject::Draw()
{
    if (monsterModel.isLoaded && !isMarkedForCleanup)
    {
        monsterModel.Draw(position, meshYaw, 1.0f, tintColor);
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
