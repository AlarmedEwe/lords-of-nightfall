#ifndef APP_H
#define APP_H

#include "Scene.h"
#include "MainMenuScene.h"
#include "UISystem.h"
#include "Settings.h"
#include "SettingsMenu.h"
#include "GameObjects/TerrainObject.h"
#include "GameObjects/PlayerObject.h"
#include "GameObjects/PickupObject.h"
#include "Item.h"

enum class GameState
{
    MainMenu,
    Gameplay
};

struct App
{
    int screenWidth = 1280;
    int screenHeight = 720;
    bool running = true;

    GameState state = GameState::MainMenu;
    bool isPaused = false;
    bool isInventoryOpen = false;
    std::string activePromptText;

    UISystem ui;
    Settings settings;
    SettingsMenu settingsMenu;

    MainMenuScene mainMenuScene;

    // Gameplay scene
    Scene scene;
    TerrainObject terrain;
    PlayerObject player;

    void Init();
    void Run();
    void Shutdown();

private:
    void StartGameplay();
    void ReturnToMainMenu();
};

#endif // APP_H
