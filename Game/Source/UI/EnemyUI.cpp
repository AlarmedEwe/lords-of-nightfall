#include "EnemyUI.h"
#include "UIWidgets.h"
#include "UISystem.h"
#include "GameObjects/EnemyObject.h"
#include "raymath.h"
#include <cstdio>

namespace EnemyUI
{

void DrawOverhead(const EnemyObject& enemy, Camera3D camera, Vector3 playerPos, UISystem& ui)
{
    if (enemy.isMarkedForCleanup || enemy.health <= 0.0f) return;

    float distToPlayer = Vector3Distance(playerPos, enemy.position);
    bool isNear = (distToPlayer <= 12.0f);
    bool isDamaged = (enemy.health < enemy.maxHealth);

    if (!isNear && !isDamaged) return;

    float headOffset = (enemy.category == "Big") ? 2.3f : ((enemy.category == "Flying") ? 2.5f : 1.6f);
    Vector3 headWorldPos = { enemy.position.x, enemy.position.y + headOffset, enemy.position.z };

    Vector2 screenPos = GetWorldToScreen(headWorldPos, camera);

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (screenPos.x < 10.0f || screenPos.x > sw - 10.0f || screenPos.y < 10.0f || screenPos.y > sh - 10.0f)
        return;

    char labelText[64];
    snprintf(labelText, sizeof(labelText), "%s [Niv. %d]", enemy.name.c_str(), enemy.level);

    float boxW = 140.0f;
    float boxH = 34.0f;
    float boxX = screenPos.x - boxW / 2.0f;
    float boxY = screenPos.y - boxH / 2.0f;

    UIWidgets::DrawPanel(ui, (Rectangle){ boxX, boxY, boxW, boxH }, (Color){ 20, 16, 28, 220 });
    UIWidgets::DrawBorder(ui, (Rectangle){ boxX, boxY, boxW, boxH }, (Color){ 180, 140, 60, 255 }, 1.5f);

    int textW = MeasureText(labelText, 10);
    DrawText(labelText, (int)(screenPos.x - textW / 2.0f), (int)(boxY + 3.0f), 10, GOLD);

    float hpPct = Clamp(enemy.health / enemy.maxHealth, 0.0f, 1.0f);
    float barW = 120.0f;
    float barH = 10.0f;
    float barX = screenPos.x - barW / 2.0f;
    float barY = boxY + 18.0f;

    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 40, 20, 20, 255 });

    Color hpColor = (hpPct > 0.5f) ? (Color){ 60, 200, 80, 255 }
                  : ((hpPct > 0.25f) ? (Color){ 230, 180, 40, 255 } : (Color){ 220, 50, 50, 255 });

    DrawRectangle((int)barX, (int)barY, (int)(barW * hpPct), (int)barH, hpColor);
    DrawRectangleLines((int)barX, (int)barY, (int)barW, (int)barH, (Color){ 100, 100, 100, 180 });
}

} // namespace EnemyUI
