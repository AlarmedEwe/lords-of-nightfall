#ifndef ENEMY_AI_H
#define ENEMY_AI_H

struct EnemyObject;
struct PlayerObject;
struct DamageFeedbackSystem;

namespace EnemyAI
{
    void Update(EnemyObject& enemy, PlayerObject* player, float deltaTime, DamageFeedbackSystem* feedback = nullptr);
}

#endif // ENEMY_AI_H
