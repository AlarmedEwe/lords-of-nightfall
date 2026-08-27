#include "PlayerEquipment.h"
#include "PlayerObject.h"
#include "raymath.h"
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

namespace PlayerEquipment
{

void SetOutfit(PlayerObject& player, const std::string& modelFile, const std::string& textureFile)
{
    player.currentOutfitModel = modelFile;
    player.currentOutfitTexture = textureFile;

    if (player.isArmorModelLoaded)
    {
        player.equippedArmorModel.Unload();
        player.isArmorModelLoaded = false;
    }

    if (!modelFile.empty() && modelFile != "Superhero_Male.lonmodel")
    {
        player.equippedArmorModel.Load(modelFile.c_str(), textureFile.c_str());
        player.isArmorModelLoaded = player.equippedArmorModel.isLoaded;
    }
}

void Sync(PlayerObject& player)
{
    // Verifica se a armadura equipada exige troca de outfit visual
    std::string targetModel = player.inventory.equippedArmor.IsValid() ? player.inventory.equippedArmor.outfitModelFile : "";
    std::string targetTex = player.inventory.equippedArmor.IsValid() ? player.inventory.equippedArmor.outfitTextureFile : "";

    if (targetModel != player.currentOutfitModel)
    {
        SetOutfit(player, targetModel, targetTex);
    }

    // Atualiza o Modelo 3D da Arma Equipada
    std::string targetWeaponPath = player.inventory.equippedWeapon.IsValid() ? player.inventory.equippedWeapon.modelFileName : "";
    if (targetWeaponPath != player.loadedWeaponModelPath)
    {
        if (player.isWeaponModelLoaded)
        {
            UnloadModel(player.equippedWeaponModel);
            player.isWeaponModelLoaded = false;
        }

        player.loadedWeaponModelPath = targetWeaponPath;
        if (!player.loadedWeaponModelPath.empty())
        {
            player.equippedWeaponModel = TryLoadModel(player.loadedWeaponModelPath.c_str());
            player.isWeaponModelLoaded = (player.equippedWeaponModel.meshCount > 0);
        }
    }
}

void Draw(PlayerObject& player, Vector3 drawPos)
{
    Color tint = (player.hitFlashTimer > 0.0f) ? (Color){ 255, 60, 60, 255 } : WHITE;

    if (player.characterModel.isLoaded)
    {
        player.characterModel.Draw(drawPos, player.meshYaw, 1.0f, tint);
    }
    else
    {
        DrawCube(player.position, 0.6f, 1.8f, 0.6f, tint);
        DrawCubeWires(player.position, 0.61f, 1.81f, 0.61f, DARKBLUE);
    }

    if (player.isArmorModelLoaded)
    {
        player.equippedArmorModel.Draw(drawPos, player.meshYaw, 1.0f, tint);
    }

    // Renderiza a Arma Equipada anexada à mão do personagem (socket da mão direita)
    if (player.isWeaponModelLoaded)
    {
        Matrix charMatrix = MatrixMultiply(MatrixRotateY(player.meshYaw * DEG2RAD), MatrixTranslate(drawPos.x, drawPos.y, drawPos.z));
        Matrix handMatrix = player.characterModel.GetRightHandMatrix();

        Matrix weaponLocalOffset = MatrixScale(0.85f, 0.85f, 0.85f);
        Matrix weaponHandMatrix = MatrixMultiply(weaponLocalOffset, handMatrix);
        Matrix weaponWorld = MatrixMultiply(weaponHandMatrix, charMatrix);

        player.equippedWeaponModel.transform = weaponWorld;
        DrawModel(player.equippedWeaponModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, tint);
        player.equippedWeaponModel.transform = MatrixIdentity();
    }
}

void Unload(PlayerObject& player)
{
    player.characterModel.Unload();
    if (player.isArmorModelLoaded)
    {
        player.equippedArmorModel.Unload();
        player.isArmorModelLoaded = false;
    }

    if (player.isWeaponModelLoaded)
    {
        UnloadModel(player.equippedWeaponModel);
        player.isWeaponModelLoaded = false;
    }
}

} // namespace PlayerEquipment
