#ifndef PLAYER_OBJECT_H
#define PLAYER_OBJECT_H

#include "../GameObject.h"

struct PlayerObject : public GameObject
{
    JPH::CharacterVirtual* character = nullptr;
    JPH::CharacterVirtualSettings* characterSettings = nullptr;

    float moveSpeed = 8.0f;
    float jumpSpeed = 10.0f;
    Vector3 velocity = {0.0f, 0.0f, 0.0f};
    bool onGround = false;

    // Direção do player baseada na câmera
    float facingYaw = 0.0f;

    void Init(Physics& physics) override;
    void Update(Physics& physics, float deltaTime) override;
    void Draw() override;
    void Shutdown(Physics& physics) override;
};

#endif // PLAYER_OBJECT_H
