#include "PlayerObject.h"
#include "raymath.h"
#include <cmath>

void PlayerObject::Init(Physics& physics)
{
    characterSettings = new JPH::CharacterVirtualSettings();
    characterSettings->mMass = 70.0f;
    characterSettings->mMaxStrength = 100.0f;
    characterSettings->mShape = new JPH::CapsuleShape(0.2f, 0.3f);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(50.0f);

    character = new JPH::CharacterVirtual(
        characterSettings,
        JPH::RVec3(0.0f, 5.0f, 0.0f),
        JPH::Quat::sIdentity(),
        0,
        &physics.system
    );

    character->SetUp(JPH::Vec3(0.0f, 1.0f, 0.0f));

    position = {0.0f, 5.0f, 0.0f};
}

void PlayerObject::Update(Physics& physics, float deltaTime)
{
    // Ground check
    onGround = character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;

    // Input de movimento
    JPH::Vec3 moveDir = JPH::Vec3::sZero();
    float yawRad = facingYaw * DEG2RAD;

    Vector3 forward = {sinf(yawRad), 0.0f, cosf(yawRad)};
    Vector3 right = {cosf(yawRad), 0.0f, -sinf(yawRad)};

    if (IsKeyDown(KEY_W)) { moveDir -= JPH::Vec3(forward.x, 0.0f, forward.z); }
    if (IsKeyDown(KEY_S)) { moveDir += JPH::Vec3(forward.x, 0.0f, forward.z); }
    if (IsKeyDown(KEY_D)) { moveDir += JPH::Vec3(right.x, 0.0f, right.z); }
    if (IsKeyDown(KEY_A)) { moveDir -= JPH::Vec3(right.x, 0.0f, right.z); }

    if (moveDir.LengthSq() > 0.0f)
        moveDir = moveDir.Normalized();

    // Velocidade horizontal
    JPH::Vec3 desiredVelocity = moveDir * moveSpeed;

    // Gravidade e pulo
    float verticalVelocity = character->GetLinearVelocity().GetY();

    if (onGround)
    {
        verticalVelocity = 0.0f;
        if (IsKeyPressed(KEY_SPACE))
            verticalVelocity = jumpSpeed;
    }
    else
    {
        verticalVelocity += physics.system.GetGravity().GetY() * deltaTime;
    }

    JPH::Vec3 newVelocity(desiredVelocity.GetX(), verticalVelocity, desiredVelocity.GetZ());
    character->SetLinearVelocity(newVelocity);

    // Configurar para o update
    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    updateSettings.mStickToFloorStepDown = JPH::Vec3(0.0f, -0.5f, 0.0f);
    updateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, 0.4f, 0.0f);

    character->ExtendedUpdate(
        deltaTime,
        physics.system.GetGravity(),
        updateSettings,
        physics.system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        physics.system.GetDefaultLayerFilter(Layers::MOVING),
        {},
        {},
        *physics.tempAllocator
    );

    // Atualizar posição do GameObject
    JPH::RVec3 charPos = character->GetPosition();
    position.x = (float)charPos.GetX();
    position.y = (float)charPos.GetY();
    position.z = (float)charPos.GetZ();
}

void PlayerObject::Draw()
{
    // A posição do CharacterVirtual é a base (pés).
    // Cápsula: meia-altura 0.2 + raio 0.3 = 0.5 total de cada lado, centro no meio.
    // Cubo visual de 1.0 de altura, centro em position.y + 0.5
    Vector3 cubePos = {position.x, position.y, position.z};
    DrawCube(cubePos, 0.6f, 1.0f, 0.6f, BLUE);
    DrawCubeWires(cubePos, 0.6f, 1.0f, 0.6f, DARKBLUE);
}

void PlayerObject::Shutdown(Physics& physics)
{
    (void)physics;
    delete character;
    character = nullptr;
    delete characterSettings;
    characterSettings = nullptr;
}
