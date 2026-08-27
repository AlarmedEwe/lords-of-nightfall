#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include "raylib.h"
#include "Physics.h"
#include "Scene.h"
#include "GameObjects/TerrainObject.h"
#include "GameObjects/NatureObject.h"
#include "GameObjects/EnemyObject.h"
#include "GameObjects/PickupObject.h"
#include "UISystem.h"
#include <string>
#include <vector>

enum class EditorTool
{
    Terraform,
    Props,
    Enemies,
    Pickups
};

struct LevelEditor
{
    bool isOpen = false;
    bool isFreeCamActive = false;

    Camera3D editorCamera = {};
    float cameraYaw = -90.0f;
    float cameraPitch = -20.0f;
    float cameraMoveSpeed = 20.0f;

    EditorTool currentTool = EditorTool::Terraform;

    // Tool 1: Terraform
    float brushRadius = 6.0f;
    float brushStrength = 5.0f;

    // Tool 2: Props
    int selectedPropIdx = 0;
    float propsScroll = 0.0f;

    // Tool 3: Enemies
    int selectedEnemyIdx = 0;
    int selectedEnemyLevel = 5;
    float enemiesScroll = 0.0f;

    // Tool 4: Pickups
    int selectedPickupIdx = 0;
    float pickupsScroll = 0.0f;

    void Init();
    void Update(Scene& scene, TerrainObject& terrain, Physics& physics, float deltaTime);
    void DrawUI(UISystem& ui, Scene& scene, TerrainObject& terrain, Physics& physics);
    void Draw3DGizmos(const Scene& scene, const TerrainObject& terrain);

    void ResetEnemies(Scene& scene);

    bool SaveLevel(const Scene& scene, const TerrainObject& terrain, const std::string& savedDir);
    bool LoadLevel(Scene& scene, TerrainObject& terrain, Physics& physics, const std::string& savedDir);
};

#endif // LEVEL_EDITOR_H

