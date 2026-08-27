#ifndef DAMAGE_FEEDBACK_SYSTEM_H
#define DAMAGE_FEEDBACK_SYSTEM_H

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

struct FloatingDamageText
{
    Vector3 worldPos;
    std::string text;
    Color color;
    float timer;
    float maxLifeTime;
    float riseSpeed;
    bool isPlayerTarget;
};

struct BloodParticle
{
    Vector3 position;
    Vector3 velocity;
    float size;
    Color color;
    float timer;
    float maxLifeTime;
};

struct DamageFeedbackSystem
{
    std::vector<FloatingDamageText> damageTexts;
    std::vector<BloodParticle> bloodParticles;

    void SpawnDamageText(Vector3 worldPos, float damage, bool isPlayerTarget);
    void SpawnBloodParticles(Vector3 pos, int count = 20);

    void Update(float deltaTime);
    void DrawParticles();
    void DrawUI(Camera3D camera);
    void Clear();
};

#endif // DAMAGE_FEEDBACK_SYSTEM_H
