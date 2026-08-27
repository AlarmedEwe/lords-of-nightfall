#include "EnemyAI.h"
#include "EnemyObject.h"
#include "PlayerObject.h"
#include "raymath.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace EnemyAI
{

void Update(EnemyObject& enemy, PlayerObject* player, float deltaTime)
{
    if (enemy.isDead || player == nullptr) return;

    float distToPlayer = Vector3Distance(enemy.position, player->position);

    // Regra dos 5 níveis: se o jogador for 5+ níveis acima e não provocado, não ataca por padrão
    bool ignoresPlayer = (player->stats.level >= enemy.stats.level + 5) && !enemy.isProvoked;

    // Se o jogador morreu, reseta provocação e volta para patrulha
    if (player->stats.health <= 0.0f)
    {
        enemy.isProvoked = false;
        enemy.state = EnemyState::PatrolIdle;
        enemy.stateTimer = 1.0f;
    }

    switch (enemy.state)
    {
    case EnemyState::HitReact:
    {
        enemy.hitReactTimer -= deltaTime;
        if (enemy.hitReactTimer <= 0.0f)
        {
            enemy.state = EnemyState::Chase;
        }
        break;
    }

    case EnemyState::PatrolIdle:
    {
        if (enemy.monsterModel.isLoaded) enemy.monsterModel.SetAnimation(0); // Idle

        enemy.stateTimer -= deltaTime;

        if (!ignoresPlayer && distToPlayer <= 12.0f)
        {
            enemy.state = EnemyState::Chase;
        }
        else if (enemy.stateTimer <= 0.0f)
        {
            float angle = ((float)(rand() % 360)) * DEG2RAD;
            float dist = 3.0f + (float)(rand() % (int)(enemy.patrolRadius * 10)) / 10.0f;

            enemy.patrolTarget = {
                enemy.spawnPos.x + cosf(angle) * dist,
                enemy.spawnPos.y,
                enemy.spawnPos.z + sinf(angle) * dist
            };

            enemy.state = EnemyState::PatrolWalk;
            enemy.stateTimer = 8.0f;
        }
        break;
    }

    case EnemyState::PatrolWalk:
    {
        if (enemy.monsterModel.isLoaded) enemy.monsterModel.SetAnimation(1); // Walk

        enemy.stateTimer -= deltaTime;

        if (!ignoresPlayer && distToPlayer <= 12.0f)
        {
            enemy.state = EnemyState::Chase;
        }
        else
        {
            Vector3 diff = { enemy.patrolTarget.x - enemy.position.x, 0.0f, enemy.patrolTarget.z - enemy.position.z };
            float distToTarget = Vector3Length(diff);

            if (distToTarget < 0.6f || enemy.stateTimer <= 0.0f)
            {
                enemy.state = EnemyState::PatrolIdle;
                enemy.stateTimer = 1.5f + (float)(rand() % 20) / 10.0f;
            }
            else
            {
                Vector3 dir = Vector3Normalize(diff);
                enemy.position.x += dir.x * enemy.walkSpeed * deltaTime;
                enemy.position.z += dir.z * enemy.walkSpeed * deltaTime;

                enemy.meshYaw = atan2f(dir.x, dir.z) * RAD2DEG;
            }
        }
        break;
    }

    case EnemyState::Chase:
    {
        if (enemy.monsterModel.isLoaded) enemy.monsterModel.SetAnimation(2); // Run

        if (distToPlayer > 22.0f)
        {
            enemy.state = EnemyState::PatrolIdle;
            enemy.stateTimer = 1.5f;
        }
        else if (distToPlayer <= 2.2f)
        {
            enemy.state = EnemyState::Attack;
            enemy.stateTimer = 0.0f;
            enemy.attackCooldownTimer = 0.0f;
        }
        else
        {
            Vector3 diff = { player->position.x - enemy.position.x, 0.0f, player->position.z - enemy.position.z };
            Vector3 dir = Vector3Normalize(diff);

            enemy.position.x += dir.x * enemy.chaseSpeed * deltaTime;
            enemy.position.z += dir.z * enemy.chaseSpeed * deltaTime;

            enemy.meshYaw = atan2f(dir.x, dir.z) * RAD2DEG;
        }
        break;
    }

    case EnemyState::Attack:
    {
        if (enemy.monsterModel.isLoaded) enemy.monsterModel.SetAnimation(3); // Attack

        enemy.stateTimer += deltaTime;
        enemy.attackCooldownTimer += deltaTime;

        // Desfere o golpe aos 0.3s de animação
        if (enemy.stateTimer >= 0.3f && enemy.stateTimer - deltaTime < 0.3f)
        {
            if (distToPlayer <= 2.6f && player->stats.health > 0.0f)
            {
                float damage = std::max(1.0f, enemy.stats.baseAttack - player->GetTotalDefense());
                player->stats.health -= damage;
                if (player->stats.health < 0.0f) player->stats.health = 0.0f;
            }
        }

        // Final do ciclo de ataque + pausa baseada em agressividade
        if (enemy.stateTimer >= (0.6f + enemy.attackPauseDuration))
        {
            if (distToPlayer <= 2.2f)
            {
                enemy.stateTimer = 0.0f;
            }
            else
            {
                enemy.state = EnemyState::Chase;
            }
        }
        break;
    }

    case EnemyState::Dead:
        break;
    }
}

} // namespace EnemyAI
