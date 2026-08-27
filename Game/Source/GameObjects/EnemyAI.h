#ifndef ENEMY_AI_H
#define ENEMY_AI_H

struct EnemyObject;
struct PlayerObject;

namespace EnemyAI
{
    void Update(EnemyObject& enemy, PlayerObject* player, float deltaTime);
}

#endif // ENEMY_AI_H
