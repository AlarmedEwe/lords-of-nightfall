#ifndef SCENE_H
#define SCENE_H

#include "raylib.h"
#include "Physics.h"
#include "GameObject.h"
#include <vector>

struct Scene
{
    Camera3D camera = {};
    std::vector<GameObject*> objects;
    Physics physics;

    // Câmera terceira pessoa
    float cameraYaw = 0.0f;
    float cameraPitch = 20.0f;
    float cameraDistance = 10.0f;
    float cameraMinDistance = 2.0f;
    float cameraMaxDistance = 20.0f;
    Vector3 cameraTarget = {0.0f, 1.0f, 0.0f};

    void Init();
    void Update(float deltaTime);
    void Draw();
    void Shutdown();

    void UpdateCamera(Vector3 playerPosition, float deltaTime, float sensitivity = 1.0f);
};

#endif // SCENE_H
