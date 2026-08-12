#ifndef MAIN_MENU_SCENE_H
#define MAIN_MENU_SCENE_H

#include "raylib.h"
#include "UISystem.h"
#include "SettingsMenu.h"
#include "GameObjects/TerrainObject.h"
#include "Physics.h"
#include "LONModel.h"

enum class MainMenuAction
{
    None,
    Play,
    Quit
};

struct MainMenuScene
{
    Camera3D camera = {};
    TerrainObject terrain;
    Physics physics;
    LONModel characterModel;

    float cameraAngle = 0.0f;
    float cameraDistance = 14.0f;
    float cameraHeight = 6.0f;
    Vector3 centerTarget = {0.0f, 1.0f, 0.0f};

    void Init();
    void Update(float deltaTime);
    void Draw(UISystem& ui, SettingsMenu& settingsMenu, Settings& settings, int screenWidth, int screenHeight, MainMenuAction& outAction);
    void Shutdown();
};

#endif // MAIN_MENU_SCENE_H
