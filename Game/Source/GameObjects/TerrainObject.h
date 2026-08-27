#ifndef TERRAIN_OBJECT_H
#define TERRAIN_OBJECT_H

#include "../GameObject.h"
#include "raylib.h"
#include <vector>
#include <cstdint>

struct LightingSystem;

struct TerrainObject : public GameObject
{
    // Heightmap
    static constexpr int GRID_SIZE = 128;
    static constexpr float TERRAIN_SCALE = 1.0f;    // espaçamento entre vértices
    static constexpr float HEIGHT_SCALE = 8.0f;     // amplitude máxima de altura
    std::vector<float> heightData;
    static TerrainObject* s_Instance;

    // Stencil / Splatmap & Texturas do Terreno
    static constexpr int SPLATMAP_RES = 256;
    std::vector<uint8_t> splatmapData;
    Texture2D splatmapTexture = {};

    Texture2D texGrass = {};
    Texture2D texDirt = {};
    Texture2D texSand = {};
    Texture2D texPavement = {};

    Shader terrainShader = {};

    // Escalas de UV por textura
    float uvScaleGrass = 25.0f;
    float uvScaleDirt = 25.0f;
    float uvScaleSand = 25.0f;
    float uvScalePavement = 25.0f;

    // Uniforms do Shader de Terreno
    int locUvScales = -1;
    int locLightDir = -1;
    int locLightColor = -1;
    int locAmbient = -1;
    int locViewPos = -1;
    int locLightVP = -1;
    int locShadowMap = -1;
    int locShadowMapRes = -1;

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

    // Inicialização de Texturas e Shaders
    void InitTextures();
    void InitShader();
    void UpdateLightingUniforms(const LightingSystem& lighting, Vector3 cameraPos);

    // Gera heightmap procedural
    void GenerateHeightmap();
    // Cria mesh raylib a partir do heightmap
    void GenerateMesh();
    // Retorna altura no ponto (x, z) do terreno
    float GetHeightAt(float x, float z);

    // Modifica altura do terreno no raio de um brush (Terraformagem)
    void ModifyHeightAt(Physics& physics, float worldX, float worldZ, float deltaHeight, float radius);
    void RebuildPhysicsShape(Physics& physics);

    // Pintura de Texturas via Stencil / Splatmap
    void PaintTextureAt(float worldX, float worldZ, int textureIndex, float strength, float radius);
    void ResetSplatmap();
    void SetSplatmapData(const std::vector<uint8_t>& data);
};

#endif // TERRAIN_OBJECT_H
