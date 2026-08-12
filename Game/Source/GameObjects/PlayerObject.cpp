#include "PlayerObject.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

void PlayerObject::Init(Physics& physics)
{
    float capsuleRadius = 0.3f;
    float capsuleHalfHeight = 0.6f;

    characterSettings = new JPH::CharacterVirtualSettings();
    characterSettings->mMass = 70.0f;
    characterSettings->mMaxStrength = 100.0f;
    characterSettings->mShape = new JPH::CapsuleShape(capsuleRadius, capsuleHalfHeight);
    characterSettings->mMaxSlopeAngle = JPH::DegreesToRadians(50.0f);
    characterSettings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -capsuleHalfHeight);

    character = new JPH::CharacterVirtual(
        characterSettings,
        JPH::RVec3(0.0f, 5.0f, 0.0f),
        JPH::Quat::sIdentity(),
        0,
        &physics.system
    );

    character->SetUp(JPH::Vec3(0.0f, 1.0f, 0.0f));
    position = {0.0f, 5.0f, 0.0f};

    // Carrega o modelo 3D lonmodel v2 do personagem Superhero Male com animações
    characterModel.Load("Superhero_Male.lonmodel", "T_Superhero_Male_Ligh.png");
}

void PlayerObject::Update(Physics& physics, float deltaTime)
{
    // Ground check do Jolt CharacterVirtual
    JPH::CharacterVirtual::EGroundState groundState = character->GetGroundState();
    onGround = (groundState == JPH::CharacterVirtual::EGroundState::OnGround) || character->IsSupported();

    // Shift para correr
    bool isRunning = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float currentSpeed = isRunning ? runSpeed : moveSpeed;

    // Vetores de direção relativos ao ângulo da câmera
    float yawRad = cameraYaw * DEG2RAD;
    Vector3 camForward = { sinf(yawRad), 0.0f, cosf(yawRad) };
    Vector3 camRight = { cosf(yawRad), 0.0f, -sinf(yawRad) };

    JPH::Vec3 moveDir = JPH::Vec3::sZero();

    // W: Frente | S: Trás | D: Direita | A: Esquerda
    if (IsKeyDown(KEY_W)) { moveDir -= JPH::Vec3(camForward.x, 0.0f, camForward.z); }
    if (IsKeyDown(KEY_S)) { moveDir += JPH::Vec3(camForward.x, 0.0f, camForward.z); }
    if (IsKeyDown(KEY_D)) { moveDir += JPH::Vec3(camRight.x, 0.0f, camRight.z); }
    if (IsKeyDown(KEY_A)) { moveDir -= JPH::Vec3(camRight.x, 0.0f, camRight.z); }

    bool isMoving = moveDir.LengthSq() > 0.001f;

    if (isMoving)
    {
        moveDir = moveDir.Normalized();

        // Rotação alvo da malha baseada no vetor de movimento
        float targetYaw = atan2f(-moveDir.GetX(), -moveDir.GetZ()) * RAD2DEG + 180.0f;

        // Interpolação suave de rotação (LerpAngle)
        float diff = fmodf(targetYaw - meshYaw + 540.0f, 360.0f) - 180.0f;
        meshYaw += diff * std::min(1.0f, deltaTime * 12.0f);
    }

    // Velocidade horizontal
    JPH::Vec3 desiredVelocity = moveDir * currentSpeed;

    // Gravidade e pulo
    float verticalVelocity = character->GetLinearVelocity().GetY();

    if (onGround)
    {
        verticalVelocity = 0.0f;
        if (IsKeyPressed(KEY_SPACE))
        {
            verticalVelocity = jumpSpeed;
            onGround = false;
        }
    }
    else
    {
        verticalVelocity += physics.system.GetGravity().GetY() * deltaTime;
    }

    JPH::Vec3 newVelocity(desiredVelocity.GetX(), verticalVelocity, desiredVelocity.GetZ());
    character->SetLinearVelocity(newVelocity);

    // Configuração de colisão com o chão
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

    // Atualiza estado de animação e reproduz no modelo
    if (!onGround)
    {
        animState = PlayerAnimState::Jump;
        characterModel.SetAnimation(3); // Jump
    }
    else if (isMoving)
    {
        if (isRunning)
        {
            animState = PlayerAnimState::Run;
            characterModel.SetAnimation(2); // Run
        }
        else
        {
            animState = PlayerAnimState::Walk;
            characterModel.SetAnimation(1); // Walk
        }
    }
    else
    {
        animState = PlayerAnimState::Idle;
        characterModel.SetAnimation(0); // Idle
    }

    characterModel.Update(deltaTime);
}

void PlayerObject::Draw()
{
    // O centro do cápsula fica a 0.9m acima dos pés. Deslocamos o modelo para que os pés fiquem perfeitamente no chão.
    float feetOffset = 0.9f;
    Vector3 drawPos = { position.x, position.y - feetOffset, position.z };

    if (characterModel.isLoaded)
    {
        characterModel.Draw(drawPos, meshYaw, 1.0f, WHITE);
    }
    else
    {
        // Fallback visual
        DrawCube(position, 0.6f, 1.8f, 0.6f, BLUE);
        DrawCubeWires(position, 0.61f, 1.81f, 0.61f, DARKBLUE);
    }
}

void PlayerObject::Shutdown(Physics& physics)
{
    (void)physics;
    characterModel.Unload();

    delete character;
    character = nullptr;
    delete characterSettings;
    characterSettings = nullptr;
}
