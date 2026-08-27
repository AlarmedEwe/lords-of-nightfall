#include "PlayerObject.h"
#include "EnemyObject.h"
#include "PlayerEquipment.h"
#include "../Item.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <vector>

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

    // Carrega o modelo 3D lonmodel do personagem com animações
    characterModel.Load(currentOutfitModel.c_str(), currentOutfitTexture.c_str());
}

void PlayerObject::Update(Physics& physics, float deltaTime)
{
    // Respawn do jogador se a vida zerar
    if (stats.IsDead())
    {
        stats.health = stats.maxHealth;
        position = { 0.0f, 5.0f, 0.0f };
        if (character)
        {
            character->SetPosition(JPH::RVec3(0.0, 5.0, 0.0));
            character->SetLinearVelocity(JPH::Vec3::sZero());
        }
    }

    // Ground check do Jolt CharacterVirtual
    JPH::CharacterVirtual::EGroundState groundState = character->GetGroundState();
    onGround = (groundState == JPH::CharacterVirtual::EGroundState::OnGround) || character->IsSupported();

    // Início / Atualização do Ataque (Botão Esquerdo do Mouse)
    if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && !isAttacking)
    {
        isAttacking = true;
        attackTimer = 0.0f;
        hasDealtDamageThisAttack = false;
    }

    if (isAttacking)
    {
        attackTimer += deltaTime;
        if (attackTimer >= attackDuration)
        {
            isAttacking = false;
            attackTimer = 0.0f;
        }
    }

    // Shift para correr
    bool isRunning = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float currentSpeed = isRunning ? runSpeed : moveSpeed;

    // Vetores de direção relativos ao ângulo da câmera
    float yawRad = cameraYaw * DEG2RAD;
    Vector3 camForward = { sinf(yawRad), 0.0f, cosf(yawRad) };
    Vector3 camRight = { cosf(yawRad), 0.0f, -sinf(yawRad) };

    JPH::Vec3 moveDir = JPH::Vec3::sZero();

    // W: Frente | S: Trás | D: Direita | A: Esquerda (apenas quando não estiver atacando)
    if (!isAttacking)
    {
        if (IsKeyDown(KEY_W)) { moveDir -= JPH::Vec3(camForward.x, 0.0f, camForward.z); }
        if (IsKeyDown(KEY_S)) { moveDir += JPH::Vec3(camForward.x, 0.0f, camForward.z); }
        if (IsKeyDown(KEY_D)) { moveDir += JPH::Vec3(camRight.x, 0.0f, camRight.z); }
        if (IsKeyDown(KEY_A)) { moveDir -= JPH::Vec3(camRight.x, 0.0f, camRight.z); }
    }

    bool isMoving = moveDir.LengthSq() > 0.001f;

    if (isMoving)
    {
        moveDir = moveDir.Normalized();

        float targetYaw = atan2f(-moveDir.GetX(), -moveDir.GetZ()) * RAD2DEG + 180.0f;
        float diff = fmodf(targetYaw - meshYaw + 540.0f, 360.0f) - 180.0f;
        meshYaw += diff * std::min(1.0f, deltaTime * 12.0f);
    }

    // Velocidade horizontal
    JPH::Vec3 desiredVelocity = moveDir * currentSpeed;
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

    // Atualiza estado de animação e reproduz nos modelos
    int currentAnimIndex = 0;
    if (isAttacking)
    {
        animState = PlayerAnimState::Attack;
        currentAnimIndex = 4;
    }
    else if (!onGround)
    {
        animState = PlayerAnimState::Jump;
        currentAnimIndex = 3;
    }
    else if (isMoving)
    {
        if (isRunning)
        {
            animState = PlayerAnimState::Run;
            currentAnimIndex = 2;
        }
        else
        {
            animState = PlayerAnimState::Walk;
            currentAnimIndex = 1;
        }
    }
    else
    {
        animState = PlayerAnimState::Idle;
        currentAnimIndex = 0;
    }

    characterModel.SetAnimation(currentAnimIndex);
    characterModel.Update(deltaTime);

    if (isArmorModelLoaded)
    {
        equippedArmorModel.SetAnimation(currentAnimIndex);
        equippedArmorModel.Update(deltaTime);
    }

    // Sincroniza equipamento visual (armadura e arma)
    PlayerEquipment::Sync(*this);
}

void PlayerObject::Draw()
{
    float feetOffset = 0.9f;
    Vector3 drawPos = { position.x, position.y - feetOffset, position.z };
    PlayerEquipment::Draw(*this, drawPos);
}

void PlayerObject::Shutdown(Physics& physics)
{
    (void)physics;
    PlayerEquipment::Unload(*this);

    delete character;
    character = nullptr;
    delete characterSettings;
    characterSettings = nullptr;
}

void PlayerObject::Heal(float amount)
{
    stats.Heal(amount);
}

void PlayerObject::AddXp(float amount)
{
    stats.AddXp(amount);
}

void PlayerObject::SetOutfit(const std::string& modelFile, const std::string& textureFile)
{
    PlayerEquipment::SetOutfit(*this, modelFile, textureFile);
}

float PlayerObject::GetTotalAttack() const
{
    return stats.baseAttack + inventory.GetTotalAttackBonus();
}

float PlayerObject::GetTotalDefense() const
{
    return stats.baseDefense + inventory.GetTotalDefenseBonus();
}

void PlayerObject::PerformAttackSweep(std::vector<GameObject*>& objects)
{
    if (!isAttacking || hasDealtDamageThisAttack) return;

    if (attackTimer >= 0.15f)
    {
        hasDealtDamageThisAttack = true;

        Vector3 attackCenter = {
            position.x + sinf(meshYaw * DEG2RAD) * 1.5f,
            position.y + 0.8f,
            position.z + cosf(meshYaw * DEG2RAD) * 1.5f
        };

        for (auto* obj : objects)
        {
            if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
            {
                if (!enemy->isDead)
                {
                    float dist = Vector3Distance(attackCenter, enemy->position);
                    if (dist <= 2.3f)
                    {
                        float dmg = std::max(1.0f, GetTotalAttack() - enemy->stats.baseDefense);
                        bool wasAlive = !enemy->isDead;

                        enemy->TakeDamage(dmg, stats.level);

                        if (wasAlive && enemy->isDead)
                        {
                            AddXp(enemy->xpReward);
                            inventory.gold += (int)enemy->goldReward;
                        }
                    }
                }
            }
        }
    }
}
