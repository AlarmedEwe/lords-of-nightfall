#include "PickupObject.h"
#include "TerrainObject.h"
#include "raymath.h"
#include <cmath>
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

PickupObject::PickupObject(Item inItem, Vector3 inPos)
{
    item = inItem;
    position = inPos;
}

void PickupObject::Init(Physics& physics)
{
    (void)physics;
    if (TerrainObject::s_Instance)
    {
        position.y = TerrainObject::s_Instance->GetHeightAt(position.x, position.z);
    }

    if (!item.modelFileName.empty())
    {
        model = TryLoadModel(item.modelFileName.c_str());
        isModelLoaded = (model.meshCount > 0);
    }
}

void PickupObject::Update(Physics& physics, float deltaTime)
{
    (void)physics;
    if (isCollected) return;

    if (TerrainObject::s_Instance)
    {
        position.y = TerrainObject::s_Instance->GetHeightAt(position.x, position.z);
    }

    rotationAngle += 45.0f * deltaTime;
    if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;

    bobbingTime += deltaTime * 2.5f;
}

void PickupObject::Draw()
{
    if (isCollected) return;

    float bobOffset = sinf(bobbingTime) * 0.12f + 0.25f;
    Vector3 drawPos = { position.x, position.y + bobOffset, position.z };

    if (isModelLoaded)
    {
        DrawModelEx(model, drawPos, (Vector3){ 0.0f, 1.0f, 0.0f }, rotationAngle, (Vector3){ 0.9f, 0.9f, 0.9f }, WHITE);
    }
    else
    {
        // Visual Fallback para Itens
        Color itemColor = GOLD;
        if (item.type == ItemType::Weapon) itemColor = RED;
        else if (item.type == ItemType::Armor) itemColor = SKYBLUE;
        else if (item.type == ItemType::Potion) itemColor = LIME;

        DrawCube(drawPos, 0.4f, 0.4f, 0.4f, itemColor);
        DrawCubeWires(drawPos, 0.42f, 0.42f, 0.42f, WHITE);
    }
}

void PickupObject::Shutdown(Physics& physics)
{
    (void)physics;
    if (isModelLoaded)
    {
        UnloadModel(model);
        isModelLoaded = false;
    }
}
