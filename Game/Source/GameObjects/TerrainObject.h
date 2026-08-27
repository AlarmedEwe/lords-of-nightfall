#ifndef TERRAIN_OBJECT_H
#define TERRAIN_OBJECT_H

#include "../GameObject.h"
#include <vector>

struct TerrainObject : public GameObject
{
    // Heightmap
    static constexpr int GRID_SIZE = 128;
    static constexpr float TERRAIN_SCALE = 1.0f;    // espaçamento entre vértices
    static constexpr float HEIGHT_SCALE = 8.0f;     // amplitude máxima de altura
    std::vector<float> heightData;
    static TerrainObject* s_Instance;

    // Rendering
    Mesh mesh = {};
    Model model = {};
    bool meshGenerated = false;

    // Physics
    JPH::BodyID bodyId;

    void Init(Physics& physics) override;
    void Update(Physics& physics, float deltaTime) override;
    void Draw() override;
    void Shutdown(Physics& physics) override;

    // Gera heightmap procedural
    void GenerateHeightmap();
    // Cria mesh raylib a partir do heightmap
    void GenerateMesh();
    // Retorna altura no ponto (x, z) do terreno
    float GetHeightAt(float x, float z);

    // Modifica altura do terreno no raio de um brush (Terraformagem)
    void ModifyHeightAt(Physics& physics, float worldX, float worldZ, float deltaHeight, float radius);
    void RebuildPhysicsShape(Physics& physics);
};

#endif // TERRAIN_OBJECT_H
