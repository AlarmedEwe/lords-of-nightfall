#ifndef ENEMY_UI_H
#define ENEMY_UI_H

#include "raylib.h"

struct EnemyObject;
struct UISystem;

namespace EnemyUI
{
    void DrawOverhead(const EnemyObject& enemy, Camera3D camera, Vector3 playerPos, UISystem& ui);
}

#endif // ENEMY_UI_H
