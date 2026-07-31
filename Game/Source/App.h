#ifndef APP_H
#define APP_H

#include "Scene.h"
#include "GameObjects/TerrainObject.h"
#include "GameObjects/PlayerObject.h"

struct App
{
    int screenWidth = 1280;
    int screenHeight = 720;
    bool running = true;

    Scene scene;
    TerrainObject terrain;
    PlayerObject player;

    void Init();
    void Run();
    void Shutdown();
};

#endif // APP_H
