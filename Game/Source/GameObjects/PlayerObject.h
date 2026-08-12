#ifndef PLAYER_OBJECT_H
#define PLAYER_OBJECT_H

#include "../GameObject.h"
#include "../LONModel.h"

enum class PlayerAnimState
{
    Idle,
    Walk,
    Run,
    Jump
};

struct PlayerObject : public GameObject
{
    JPH::CharacterVirtual* character = nullptr;
    JPH::CharacterVirtualSettings* characterSettings = nullptr;

    float moveSpeed = 8.0f;
    float runSpeed = 13.0f;
    float jumpSpeed = 10.0f;
    Vector3 velocity = {0.0f, 0.0f, 0.0f};
    bool onGround = false;

    // Ângulo da câmera e rotação visual do personagem
    float cameraYaw = 0.0f;
    float meshYaw = 180.0f;

    // Modelo 3D do personagem Superhero
    LONModel characterModel;
    PlayerAnimState animState = PlayerAnimState::Idle;
    float animTime = 0.0f;

    void Init(Physics& physics) override;
    void Update(Physics& physics, float deltaTime) override;
    void Draw() override;
    void Shutdown(Physics& physics) override;
};

#endif // PLAYER_OBJECT_H
