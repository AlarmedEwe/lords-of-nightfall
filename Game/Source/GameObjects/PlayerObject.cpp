#include "PlayerObject.h"
#include "../Item.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <vector>

static Model TryLoadModel(const char* relPath)
{
    if (!relPath || relPath[0] == '\0') return (Model){ 0 };

    std::vector<std::string> candidatePaths = {
        relPath,
        std::string("Assets/") + relPath,
        std::string("Build/Development/Assets/") + relPath
    };

    for (const std::string& path : candidatePaths)
    {
        if (FileExists(path.c_str()))
        {
            Model m = LoadModel(path.c_str());
            if (m.meshCount > 0)
                return m;
        }
    }
    return (Model){ 0 };
}

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
    // Ground check do Jolt CharacterVirtual
    JPH::CharacterVirtual::EGroundState groundState = character->GetGroundState();
    onGround = (groundState == JPH::CharacterVirtual::EGroundState::OnGround) || character->IsSupported();

    // Início / Atualização do Ataque (Botão Esquerdo do Mouse)
    if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && !isAttacking)
    {
        isAttacking = true;
        attackTimer = 0.0f;
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

    // Atualiza estado de animação e reproduz nos modelos
    int currentAnimIndex = 0;
    if (isAttacking)
    {
        animState = PlayerAnimState::Attack;
        currentAnimIndex = 4; // Attack
    }
    else if (!onGround)
    {
        animState = PlayerAnimState::Jump;
        currentAnimIndex = 3; // Jump
    }
    else if (isMoving)
    {
        if (isRunning)
        {
            animState = PlayerAnimState::Run;
            currentAnimIndex = 2; // Run
        }
        else
        {
            animState = PlayerAnimState::Walk;
            currentAnimIndex = 1; // Walk
        }
    }
    else
    {
        animState = PlayerAnimState::Idle;
        currentAnimIndex = 0; // Idle (sempre Idle padrão em pé)
    }

    characterModel.SetAnimation(currentAnimIndex);
    characterModel.Update(deltaTime);

    if (isArmorModelLoaded)
    {
        equippedArmorModel.SetAnimation(currentAnimIndex);
        equippedArmorModel.Update(deltaTime);
    }

    // Verifica se a armadura equipada exige troca de outfit visual
    std::string targetModel = inventory.equippedArmor.IsValid() ? inventory.equippedArmor.outfitModelFile : "";
    std::string targetTex = inventory.equippedArmor.IsValid() ? inventory.equippedArmor.outfitTextureFile : "";

    if (targetModel != currentOutfitModel)
    {
        SetOutfit(targetModel, targetTex);
    }

    // Atualiza o Modelo 3D da Arma Equipada
    std::string targetWeaponPath = inventory.equippedWeapon.IsValid() ? inventory.equippedWeapon.modelFileName : "";
    if (targetWeaponPath != loadedWeaponModelPath)
    {
        if (isWeaponModelLoaded)
        {
            UnloadModel(equippedWeaponModel);
            isWeaponModelLoaded = false;
        }

        loadedWeaponModelPath = targetWeaponPath;
        if (!loadedWeaponModelPath.empty())
        {
            equippedWeaponModel = TryLoadModel(loadedWeaponModelPath.c_str());
            isWeaponModelLoaded = (equippedWeaponModel.meshCount > 0);
        }
    }
}

void PlayerObject::Draw()
{
    float feetOffset = 0.9f;
    Vector3 drawPos = { position.x, position.y - feetOffset, position.z };

    if (characterModel.isLoaded)
    {
        characterModel.Draw(drawPos, meshYaw, 1.0f, WHITE);
    }
    else
    {
        DrawCube(position, 0.6f, 1.8f, 0.6f, BLUE);
        DrawCubeWires(position, 0.61f, 1.81f, 0.61f, DARKBLUE);
    }

    if (isArmorModelLoaded)
    {
        equippedArmorModel.Draw(drawPos, meshYaw, 1.0f, WHITE);
    }

    // Renderiza a Arma Equipada anexada à mão do personagem (socket da mão direita)
    if (isWeaponModelLoaded)
    {
        Matrix charMatrix = MatrixMultiply(MatrixRotateY(meshYaw * DEG2RAD), MatrixTranslate(drawPos.x, drawPos.y, drawPos.z));
        Matrix handMatrix = characterModel.GetRightHandMatrix();

        // Offset local do modelo da arma (escala e ajuste para a palma da mão)
        Matrix weaponLocalOffset = MatrixScale(0.85f, 0.85f, 0.85f);
        Matrix weaponHandMatrix = MatrixMultiply(weaponLocalOffset, handMatrix);
        Matrix weaponWorld = MatrixMultiply(weaponHandMatrix, charMatrix);

        equippedWeaponModel.transform = weaponWorld;
        DrawModel(equippedWeaponModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
        equippedWeaponModel.transform = MatrixIdentity();
    }
}

void PlayerObject::Shutdown(Physics& physics)
{
    (void)physics;
    characterModel.Unload();
    if (isArmorModelLoaded)
    {
        equippedArmorModel.Unload();
        isArmorModelLoaded = false;
    }

    if (isWeaponModelLoaded)
    {
        UnloadModel(equippedWeaponModel);
        isWeaponModelLoaded = false;
    }

    delete character;
    character = nullptr;
    delete characterSettings;
    characterSettings = nullptr;
}

void PlayerObject::Heal(float amount)
{
    health = std::min(maxHealth, health + amount);
}

void PlayerObject::AddXp(float amount)
{
    xp += amount;
    while (xp >= maxXp)
    {
        xp -= maxXp;
        level++;
        maxXp *= 1.25f;
        maxHealth += 20.0f;
        health = maxHealth;
        baseAttack += 5.0f;
        baseDefense += 2.0f;
    }
}

void PlayerObject::SetOutfit(const std::string& modelFile, const std::string& textureFile)
{
    currentOutfitModel = modelFile;
    currentOutfitTexture = textureFile;

    if (isArmorModelLoaded)
    {
        equippedArmorModel.Unload();
        isArmorModelLoaded = false;
    }

    if (!modelFile.empty() && modelFile != "Superhero_Male.lonmodel")
    {
        equippedArmorModel.Load(modelFile.c_str(), textureFile.c_str());
        isArmorModelLoaded = equippedArmorModel.isLoaded;
    }
}

float PlayerObject::GetTotalAttack() const
{
    return baseAttack + inventory.GetTotalAttackBonus();
}

float PlayerObject::GetTotalDefense() const
{
    return baseDefense + inventory.GetTotalDefenseBonus();
}
