#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "raylib.h"
#include "Physics.h"

struct GameObject
{
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 rotation = {0.0f, 0.0f, 0.0f};
    Vector3 scale = {1.0f, 1.0f, 1.0f};
    bool active = true;

    virtual ~GameObject() = default;

    virtual void Init(Physics& physics) { (void)physics; }
    virtual void Update(Physics& physics, float deltaTime) { (void)physics; (void)deltaTime; }
    virtual void Draw() {}
    virtual void Shutdown(Physics& physics) { (void)physics; }
};

#endif // GAME_OBJECT_H
