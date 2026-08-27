#include "DamageFeedbackSystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstdio>

void DamageFeedbackSystem::SpawnDamageText(Vector3 worldPos, float damage, bool isPlayerTarget)
{
    FloatingDamageText dt;
    float offsetX = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.6f;
    float offsetZ = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.6f;

    dt.worldPos = { worldPos.x + offsetX, worldPos.y, worldPos.z + offsetZ };

    int dmgValue = (int)roundf(damage);
    dt.text = std::to_string(dmgValue);

    if (isPlayerTarget)
    {
        dt.color = (Color){ 255, 50, 50, 255 };
    }
    else
    {
        dt.color = (Color){ 255, 210, 40, 255 };
    }

    dt.maxLifeTime = 1.0f;
    dt.timer = 1.0f;
    dt.riseSpeed = 1.8f;
    dt.isPlayerTarget = isPlayerTarget;

    damageTexts.push_back(dt);
}

void DamageFeedbackSystem::SpawnBloodParticles(Vector3 pos, int count)
{
    for (int i = 0; i < count; i++)
    {
        BloodParticle p;
        float jitterX = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.3f;
        float jitterY = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.3f;
        float jitterZ = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.3f;

        p.position = { pos.x + jitterX, pos.y + jitterY, pos.z + jitterZ };

        float angle = ((float)(rand() % 360)) * DEG2RAD;
        float horizSpeed = 1.2f + (float)(rand() % 100) / 100.0f * 3.0f;
        float vertSpeed = 1.5f + (float)(rand() % 100) / 100.0f * 3.5f;

        p.velocity = { cosf(angle) * horizSpeed, vertSpeed, sinf(angle) * horizSpeed };
        p.size = 0.07f + (float)(rand() % 100) / 1000.0f;

        int colorType = rand() % 3;
        if (colorType == 0)
            p.color = (Color){ (unsigned char)(210 + rand() % 45), (unsigned char)(10 + rand() % 25), (unsigned char)(10 + rand() % 25), 255 };
        else if (colorType == 1)
            p.color = (Color){ (unsigned char)(150 + rand() % 40), (unsigned char)(5 + rand() % 15), (unsigned char)(5 + rand() % 15), 255 };
        else
            p.color = (Color){ (unsigned char)(100 + rand() % 30), 0, 0, 255 };

        p.maxLifeTime = 0.4f + (float)(rand() % 100) / 200.0f;
        p.timer = p.maxLifeTime;

        bloodParticles.push_back(p);
    }
}

void DamageFeedbackSystem::Update(float deltaTime)
{
    for (size_t i = 0; i < damageTexts.size(); )
    {
        damageTexts[i].worldPos.y += damageTexts[i].riseSpeed * deltaTime;
        damageTexts[i].timer -= deltaTime;

        if (damageTexts[i].timer <= 0.0f)
        {
            damageTexts.erase(damageTexts.begin() + i);
        }
        else
        {
            i++;
        }
    }

    for (size_t i = 0; i < bloodParticles.size(); )
    {
        bloodParticles[i].position.x += bloodParticles[i].velocity.x * deltaTime;
        bloodParticles[i].position.y += bloodParticles[i].velocity.y * deltaTime;
        bloodParticles[i].position.z += bloodParticles[i].velocity.z * deltaTime;

        bloodParticles[i].velocity.y -= 13.0f * deltaTime;
        bloodParticles[i].timer -= deltaTime;

        if (bloodParticles[i].timer <= 0.0f || bloodParticles[i].position.y < 0.0f)
        {
            bloodParticles.erase(bloodParticles.begin() + i);
        }
        else
        {
            i++;
        }
    }
}

void DamageFeedbackSystem::DrawParticles()
{
    for (const auto& p : bloodParticles)
    {
        float alphaPct = p.timer / p.maxLifeTime;
        Color c = p.color;
        c.a = (unsigned char)(255.0f * std::clamp(alphaPct * 2.0f, 0.0f, 1.0f));

        DrawCube(p.position, p.size, p.size, p.size, c);
    }
}

void DamageFeedbackSystem::DrawUI(Camera3D camera)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    for (const auto& dt : damageTexts)
    {
        Vector3 camToTarget = Vector3Subtract(dt.worldPos, camera.position);
        Vector3 camForward = Vector3Subtract(camera.target, camera.position);
        if (Vector3DotProduct(camToTarget, camForward) <= 0.0f) continue;

        Vector2 screenPos = GetWorldToScreen(dt.worldPos, camera);

        if (screenPos.x < -100.0f || screenPos.x > sw + 100.0f || screenPos.y < -100.0f || screenPos.y > sh + 100.0f)
            continue;

        float alpha = std::clamp(dt.timer / (dt.maxLifeTime * 0.4f), 0.0f, 1.0f);
        Color drawColor = dt.color;
        drawColor.a = (unsigned char)(255.0f * alpha);

        Color shadowColor = (Color){ 0, 0, 0, (unsigned char)(220.0f * alpha) };

        int baseFontSize = dt.isPlayerTarget ? 26 : 24;
        float progress = 1.0f - (dt.timer / dt.maxLifeTime);
        float scale = (progress < 0.15f) ? (1.3f - progress * 2.0f) : 1.0f;
        int fontSize = (int)(baseFontSize * scale);

        int textW = MeasureText(dt.text.c_str(), fontSize);
        int posX = (int)(screenPos.x - textW / 2.0f);
        int posY = (int)(screenPos.y);

        DrawText(dt.text.c_str(), posX + 2, posY + 2, fontSize, shadowColor);
        DrawText(dt.text.c_str(), posX, posY, fontSize, drawColor);
    }
}

void DamageFeedbackSystem::Clear()
{
    damageTexts.clear();
    bloodParticles.clear();
}
