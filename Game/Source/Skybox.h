#ifndef SKYBOX_H
#define SKYBOX_H

#include "raylib.h"

struct Skybox
{
    Mesh cubeMesh = {};
    Model skyModel = {};
    Shader skyShader = {};
    int lightDirLoc = -1;
    bool isLoaded = false;

    void Init();
    void Draw(Camera camera, Vector3 lightDir);
    void Shutdown();
};

#endif // SKYBOX_H
