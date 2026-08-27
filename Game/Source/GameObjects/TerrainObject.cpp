#include "TerrainObject.h"
#include "../LightingSystem.h"
#include "rlgl.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstring>

TerrainObject* TerrainObject::s_Instance = nullptr;

// Simplex-like noise usando múltiplas oitavas de sin
static float Noise2D(float x, float z)
{
    float value = 0.0f;
    value += sinf(x * 0.05f) * cosf(z * 0.05f) * 0.5f;
    value += sinf(x * 0.1f + 1.3f) * cosf(z * 0.08f + 2.7f) * 0.25f;
    value += sinf(x * 0.2f + 5.1f) * cosf(z * 0.15f + 3.2f) * 0.125f;
    value += sinf(x * 0.4f + 8.3f) * cosf(z * 0.35f + 1.9f) * 0.0625f;
    return value;
}

static Texture2D TryLoadTerrainTexture(const char* filename)
{
    const char* paths[] = {
        TextFormat("Terrain/%s", filename),
        TextFormat("Assets/Terrain/%s", filename),
        TextFormat("Build/Development/Assets/Terrain/%s", filename),
        TextFormat("Game/Assets/Terrain/%s", filename)
    };
    for (const char* p : paths)
    {
        if (FileExists(p))
        {
            Texture2D tex = LoadTexture(p);
            if (tex.id > 0)
            {
                SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
                SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                GenTextureMipmaps(&tex);
                TraceLog(LOG_INFO, "[Terrain] Textura carregada com sucesso: %s (ID: %u, %dx%d)", p, tex.id, tex.width, tex.height);
                return tex;
            }
        }
    }
    TraceLog(LOG_WARNING, "[Terrain] Textura NAO encontrada: %s", filename);
    return (Texture2D){ 0 };
}

static Shader TryLoadTerrainShader()
{
    const char* vsPaths[] = {
        "Shaders/terrain.vs",
        "Assets/Shaders/terrain.vs",
        "Build/Development/Assets/Shaders/terrain.vs",
        "Game/Assets/Shaders/terrain.vs"
    };

    const char* fsPaths[] = {
        "Shaders/terrain.fs",
        "Assets/Shaders/terrain.fs",
        "Build/Development/Assets/Shaders/terrain.fs",
        "Game/Assets/Shaders/terrain.fs"
    };

    const char* foundVS = nullptr;
    const char* foundFS = nullptr;

    for (const char* p : vsPaths)
    {
        if (FileExists(p)) { foundVS = p; break; }
    }
    for (const char* p : fsPaths)
    {
        if (FileExists(p)) { foundFS = p; break; }
    }

    if (foundVS && foundFS)
    {
        return LoadShader(foundVS, foundFS);
    }
    TraceLog(LOG_WARNING, "[Terrain] Shader de terreno nao encontrado, usando shader padrao.");
    return (Shader){ 0 };
}

void TerrainObject::InitTextures()
{
    // Carrega as 4 texturas de terreno da pasta Game/Assets/Terrain
    texGrass = TryLoadTerrainTexture("grass.png");
    texDirt = TryLoadTerrainTexture("dirt.png");
    texSand = TryLoadTerrainTexture("sand.png");
    texPavement = TryLoadTerrainTexture("pavement.png");

    // Inicializa o Splatmap / Stencil Map (256x256 RGBA, padrão 100% Grama: R=255, G=0, B=0, A=0)
    if (splatmapData.empty() || splatmapData.size() != (size_t)(SPLATMAP_RES * SPLATMAP_RES * 4))
    {
        ResetSplatmap();
    }
    else
    {
        Image img = {
            .data = splatmapData.data(),
            .width = SPLATMAP_RES,
            .height = SPLATMAP_RES,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        splatmapTexture = LoadTextureFromImage(img);
        SetTextureWrap(splatmapTexture, TEXTURE_WRAP_CLAMP);
        SetTextureFilter(splatmapTexture, TEXTURE_FILTER_BILINEAR);
    }
}

void TerrainObject::ResetSplatmap()
{
    splatmapData.resize(SPLATMAP_RES * SPLATMAP_RES * 4);
    for (int i = 0; i < SPLATMAP_RES * SPLATMAP_RES; i++)
    {
        splatmapData[i * 4 + 0] = 255; // 100% Grama (Canal R)
        splatmapData[i * 4 + 1] = 0;   // 0% Terra (Canal G)
        splatmapData[i * 4 + 2] = 0;   // 0% Areia (Canal B)
        splatmapData[i * 4 + 3] = 0;   // 0% Pavimento (Canal A)
    }

    if (splatmapTexture.id > 0)
    {
        UpdateTexture(splatmapTexture, splatmapData.data());
    }
    else
    {
        Image img = {
            .data = splatmapData.data(),
            .width = SPLATMAP_RES,
            .height = SPLATMAP_RES,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        };
        splatmapTexture = LoadTextureFromImage(img);
        SetTextureWrap(splatmapTexture, TEXTURE_WRAP_CLAMP);
        SetTextureFilter(splatmapTexture, TEXTURE_FILTER_BILINEAR);
    }
}

void TerrainObject::SetSplatmapData(const std::vector<uint8_t>& data)
{
    if (data.size() == (size_t)(SPLATMAP_RES * SPLATMAP_RES * 4))
    {
        splatmapData = data;
        if (splatmapTexture.id > 0)
        {
            UpdateTexture(splatmapTexture, splatmapData.data());
        }
    }
}

void TerrainObject::InitShader()
{
    terrainShader = TryLoadTerrainShader();

    if (terrainShader.id > 0)
    {
        locUvScales = GetShaderLocation(terrainShader, "uvScales");
        locLightDir = GetShaderLocation(terrainShader, "lightDir");
        locLightColor = GetShaderLocation(terrainShader, "lightColor");
        locAmbient = GetShaderLocation(terrainShader, "ambient");
        locViewPos = GetShaderLocation(terrainShader, "viewPos");
        locLightVP = GetShaderLocation(terrainShader, "lightVP");
        locShadowMap = GetShaderLocation(terrainShader, "shadowMap");
        locShadowMapRes = GetShaderLocation(terrainShader, "shadowMapResolution");

        int locSplat = GetShaderLocation(terrainShader, "splatmap");
        int locGrass = GetShaderLocation(terrainShader, "texGrass");
        int locDirt  = GetShaderLocation(terrainShader, "texDirt");
        int locSand  = GetShaderLocation(terrainShader, "texSand");
        int locPave  = GetShaderLocation(terrainShader, "texPavement");

        terrainShader.locs[SHADER_LOC_MAP_DIFFUSE]   = locSplat;
        terrainShader.locs[SHADER_LOC_MAP_SPECULAR]  = locShadowMap;
        terrainShader.locs[SHADER_LOC_MAP_NORMAL]    = locGrass;
        terrainShader.locs[SHADER_LOC_MAP_ROUGHNESS] = locDirt;
        terrainShader.locs[SHADER_LOC_MAP_OCCLUSION] = locSand;
        terrainShader.locs[SHADER_LOC_MAP_EMISSION]  = locPave;

        int s0 = 0, s1 = 1, s2 = 2, s3 = 3, s4 = 4, s5 = 5;
        if (locSplat >= 0) SetShaderValue(terrainShader, locSplat, &s0, SHADER_UNIFORM_INT);
        if (locShadowMap >= 0) SetShaderValue(terrainShader, locShadowMap, &s1, SHADER_UNIFORM_INT);
        if (locGrass >= 0) SetShaderValue(terrainShader, locGrass, &s2, SHADER_UNIFORM_INT);
        if (locDirt >= 0) SetShaderValue(terrainShader, locDirt, &s3, SHADER_UNIFORM_INT);
        if (locSand >= 0) SetShaderValue(terrainShader, locSand, &s4, SHADER_UNIFORM_INT);
        if (locPave >= 0) SetShaderValue(terrainShader, locPave, &s5, SHADER_UNIFORM_INT);

        if (model.materialCount > 0)
        {
            model.materials[0].shader = terrainShader;
            model.materials[0].maps[0].texture = splatmapTexture;
            model.materials[0].maps[2].texture = texGrass;
            model.materials[0].maps[3].texture = texDirt;
            model.materials[0].maps[4].texture = texSand;
            model.materials[0].maps[5].texture = texPavement;
        }

        TraceLog(LOG_INFO, "[Terrain] Shader de stencil/splatmap e iluminação configurado com sucesso.");
    }
}

void TerrainObject::UpdateLightingUniforms(const LightingSystem& lighting, Vector3 cameraPos)
{
    if (terrainShader.id == 0) return;

    Vector3 normLightDir = Vector3Normalize(lighting.lightDir);
    int slot1 = 1;

    SetShaderValue(terrainShader, locLightDir, &normLightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(terrainShader, locLightColor, &lighting.lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(terrainShader, locAmbient, &lighting.ambientColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(terrainShader, locViewPos, &cameraPos, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(terrainShader, locLightVP, lighting.lightViewProj);
    SetShaderValue(terrainShader, locShadowMapRes, &lighting.shadowMapResolution, SHADER_UNIFORM_INT);
    SetShaderValue(terrainShader, locShadowMap, &slot1, SHADER_UNIFORM_INT);

    if (model.materialCount > 0)
    {
        model.materials[0].maps[1].texture = lighting.shadowMap.depth;
    }
}

void TerrainObject::GenerateHeightmap()
{
    heightData.resize(GRID_SIZE * GRID_SIZE);

    for (int z = 0; z < GRID_SIZE; z++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            float worldX = (float)x * TERRAIN_SCALE;
            float worldZ = (float)z * TERRAIN_SCALE;
            heightData[z * GRID_SIZE + x] = Noise2D(worldX, worldZ) * HEIGHT_SCALE;
        }
    }
}

void TerrainObject::GenerateMesh()
{
    int vertCount = GRID_SIZE * GRID_SIZE;
    int triCount = (GRID_SIZE - 1) * (GRID_SIZE - 1) * 2;

    mesh.vertexCount = vertCount;
    mesh.triangleCount = triCount;
    mesh.vertices = (float*)MemAlloc(sizeof(float) * 3 * vertCount);
    mesh.normals = (float*)MemAlloc(sizeof(float) * 3 * vertCount);
    mesh.texcoords = (float*)MemAlloc(sizeof(float) * 2 * vertCount);
    mesh.indices = (unsigned short*)MemAlloc(sizeof(unsigned short) * 3 * triCount);

    // Offset para centralizar o terreno
    float offsetX = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float offsetZ = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;

    // Vértices
    for (int z = 0; z < GRID_SIZE; z++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            int i = z * GRID_SIZE + x;
            mesh.vertices[i * 3 + 0] = (float)x * TERRAIN_SCALE + offsetX;
            mesh.vertices[i * 3 + 1] = heightData[i];
            mesh.vertices[i * 3 + 2] = (float)z * TERRAIN_SCALE + offsetZ;

            mesh.texcoords[i * 2 + 0] = (float)x / (float)(GRID_SIZE - 1);
            mesh.texcoords[i * 2 + 1] = (float)z / (float)(GRID_SIZE - 1);
        }
    }

    // Índices
    int idx = 0;
    for (int z = 0; z < GRID_SIZE - 1; z++)
    {
        for (int x = 0; x < GRID_SIZE - 1; x++)
        {
            int topLeft = z * GRID_SIZE + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * GRID_SIZE + x;
            int bottomRight = bottomLeft + 1;

            mesh.indices[idx++] = (unsigned short)topLeft;
            mesh.indices[idx++] = (unsigned short)bottomLeft;
            mesh.indices[idx++] = (unsigned short)topRight;

            mesh.indices[idx++] = (unsigned short)topRight;
            mesh.indices[idx++] = (unsigned short)bottomLeft;
            mesh.indices[idx++] = (unsigned short)bottomRight;
        }
    }

    // Normais (média de faces adjacentes)
    for (int i = 0; i < vertCount * 3; i++) mesh.normals[i] = 0.0f;

    for (int i = 0; i < triCount; i++)
    {
        int i0 = mesh.indices[i * 3 + 0];
        int i1 = mesh.indices[i * 3 + 1];
        int i2 = mesh.indices[i * 3 + 2];

        Vector3 v0 = {mesh.vertices[i0*3], mesh.vertices[i0*3+1], mesh.vertices[i0*3+2]};
        Vector3 v1 = {mesh.vertices[i1*3], mesh.vertices[i1*3+1], mesh.vertices[i1*3+2]};
        Vector3 v2 = {mesh.vertices[i2*3], mesh.vertices[i2*3+1], mesh.vertices[i2*3+2]};

        Vector3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
        Vector3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

        Vector3 normal = {
            edge1.y * edge2.z - edge1.z * edge2.y,
            edge1.z * edge2.x - edge1.x * edge2.z,
            edge1.x * edge2.y - edge1.y * edge2.x
        };

        mesh.normals[i0*3+0] += normal.x; mesh.normals[i0*3+1] += normal.y; mesh.normals[i0*3+2] += normal.z;
        mesh.normals[i1*3+0] += normal.x; mesh.normals[i1*3+1] += normal.y; mesh.normals[i1*3+2] += normal.z;
        mesh.normals[i2*3+0] += normal.x; mesh.normals[i2*3+1] += normal.y; mesh.normals[i2*3+2] += normal.z;
    }

    // Normalizar
    for (int i = 0; i < vertCount; i++)
    {
        float nx = mesh.normals[i*3+0];
        float ny = mesh.normals[i*3+1];
        float nz = mesh.normals[i*3+2];
        float len = sqrtf(nx*nx + ny*ny + nz*nz);
        if (len > 0.0f)
        {
            mesh.normals[i*3+0] /= len;
            mesh.normals[i*3+1] /= len;
            mesh.normals[i*3+2] /= len;
        }
    }

    UploadMesh(&mesh, true);
    model = LoadModelFromMesh(mesh);
    meshGenerated = true;
}

void TerrainObject::Init(Physics& physics)
{
    s_Instance = this;
    GenerateHeightmap();
    GenerateMesh();
    InitTextures();
    InitShader();

    // Criar HeightFieldShape no Jolt
    float offsetX = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float offsetZ = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;

    JPH::HeightFieldShapeSettings settings(
        heightData.data(),
        JPH::Vec3(offsetX, 0.0f, offsetZ),
        JPH::Vec3(TERRAIN_SCALE, 1.0f, TERRAIN_SCALE),
        GRID_SIZE
    );

    JPH::ShapeSettings::ShapeResult shapeResult = settings.Create();
    if (!shapeResult.IsValid())
    {
        TraceLog(LOG_ERROR, "[Terrain] Falha ao criar HeightFieldShape");
        return;
    }

    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        JPH::RVec3::sZero(),
        JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Layers::NON_MOVING
    );

    JPH::BodyInterface& bodyInterface = physics.system.GetBodyInterface();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (body)
    {
        bodyId = body->GetID();
        bodyInterface.AddBody(bodyId, JPH::EActivation::DontActivate);
    }
}

void TerrainObject::Update(Physics& physics, float deltaTime)
{
    (void)physics;
    (void)deltaTime;
}

void TerrainObject::Draw()
{
    if (!meshGenerated) return;

    if (terrainShader.id > 0)
    {
        int s0 = 0, s1 = 1, s2 = 2, s3 = 3, s4 = 4, s5 = 5;
        int locSplat = GetShaderLocation(terrainShader, "splatmap");
        int locGrass = GetShaderLocation(terrainShader, "texGrass");
        int locDirt  = GetShaderLocation(terrainShader, "texDirt");
        int locSand  = GetShaderLocation(terrainShader, "texSand");
        int locPave  = GetShaderLocation(terrainShader, "texPavement");

        if (locSplat >= 0) SetShaderValue(terrainShader, locSplat, &s0, SHADER_UNIFORM_INT);
        if (locShadowMap >= 0) SetShaderValue(terrainShader, locShadowMap, &s1, SHADER_UNIFORM_INT);
        if (locGrass >= 0) SetShaderValue(terrainShader, locGrass, &s2, SHADER_UNIFORM_INT);
        if (locDirt >= 0) SetShaderValue(terrainShader, locDirt, &s3, SHADER_UNIFORM_INT);
        if (locSand >= 0) SetShaderValue(terrainShader, locSand, &s4, SHADER_UNIFORM_INT);
        if (locPave >= 0) SetShaderValue(terrainShader, locPave, &s5, SHADER_UNIFORM_INT);

        if (locUvScales >= 0)
        {
            Vector4 uvScalesVec = { uvScaleGrass, uvScaleDirt, uvScaleSand, uvScalePavement };
            SetShaderValue(terrainShader, locUvScales, &uvScalesVec, SHADER_UNIFORM_VEC4);
        }

        // Ativação explícita de todas as 6 unidades de textura OpenGL no rlgl
        rlActiveTextureSlot(0);
        rlEnableTexture(splatmapTexture.id);

        rlActiveTextureSlot(1);
        if (model.materials[0].maps[1].texture.id > 0)
            rlEnableTexture(model.materials[0].maps[1].texture.id);

        rlActiveTextureSlot(2);
        rlEnableTexture(texGrass.id);

        rlActiveTextureSlot(3);
        rlEnableTexture(texDirt.id);

        rlActiveTextureSlot(4);
        rlEnableTexture(texSand.id);

        rlActiveTextureSlot(5);
        rlEnableTexture(texPavement.id);
    }

    DrawModel(model, position, 1.0f, WHITE);

    if (terrainShader.id > 0)
    {
        rlActiveTextureSlot(5); rlDisableTexture();
        rlActiveTextureSlot(4); rlDisableTexture();
        rlActiveTextureSlot(3); rlDisableTexture();
        rlActiveTextureSlot(2); rlDisableTexture();
        rlActiveTextureSlot(1); rlDisableTexture();
        rlActiveTextureSlot(0); rlDisableTexture();
    }
}

void TerrainObject::Shutdown(Physics& physics)
{
    if (s_Instance == this) s_Instance = nullptr;
    JPH::BodyInterface& bodyInterface = physics.system.GetBodyInterface();
    bodyInterface.RemoveBody(bodyId);
    bodyInterface.DestroyBody(bodyId);

    if (meshGenerated)
    {
        UnloadModel(model);
        meshGenerated = false;
    }

    if (splatmapTexture.id > 0) UnloadTexture(splatmapTexture);
    if (texGrass.id > 0) UnloadTexture(texGrass);
    if (texDirt.id > 0) UnloadTexture(texDirt);
    if (texSand.id > 0) UnloadTexture(texSand);
    if (texPavement.id > 0) UnloadTexture(texPavement);
    if (terrainShader.id > 0) UnloadShader(terrainShader);
}

float TerrainObject::GetHeightAt(float x, float z)
{
    if (heightData.empty() || heightData.size() < (size_t)(GRID_SIZE * GRID_SIZE))
        return 0.0f;

    float offsetX = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float offsetZ = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;

    float localX = (x - offsetX) / TERRAIN_SCALE;
    float localZ = (z - offsetZ) / TERRAIN_SCALE;

    int ix = (int)localX;
    int iz = (int)localZ;

    if (ix < 0 || ix >= GRID_SIZE - 1 || iz < 0 || iz >= GRID_SIZE - 1)
        return 0.0f;

    float fx = localX - (float)ix;
    float fz = localZ - (float)iz;

    float h00 = heightData[iz * GRID_SIZE + ix];
    float h10 = heightData[iz * GRID_SIZE + ix + 1];
    float h01 = heightData[(iz + 1) * GRID_SIZE + ix];
    float h11 = heightData[(iz + 1) * GRID_SIZE + ix + 1];

    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;

    return h0 * (1.0f - fz) + h1 * fz;
}

void TerrainObject::ModifyHeightAt(Physics& physics, float worldX, float worldZ, float deltaHeight, float radius)
{
    if (heightData.size() < (size_t)(GRID_SIZE * GRID_SIZE)) return;

    float offsetX = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float offsetZ = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;

    bool modified = false;

    for (int z = 0; z < GRID_SIZE; z++)
    {
        for (int x = 0; x < GRID_SIZE; x++)
        {
            float vx = (float)x * TERRAIN_SCALE + offsetX;
            float vz = (float)z * TERRAIN_SCALE + offsetZ;

            float dx = vx - worldX;
            float dz = vz - worldZ;
            float distSq = dx * dx + dz * dz;

            if (distSq <= radius * radius)
            {
                float dist = sqrtf(distSq);
                float falloff = cosf((dist / radius) * (3.14159265f * 0.5f));
                if (falloff < 0.0f) falloff = 0.0f;

                int idx = z * GRID_SIZE + x;
                heightData[idx] += deltaHeight * falloff;
                modified = true;

                if (meshGenerated && mesh.vertices)
                {
                    mesh.vertices[idx * 3 + 1] = heightData[idx];
                }
            }
        }
    }

    if (modified)
    {
        if (meshGenerated && mesh.vertices)
        {
            UpdateMeshBuffer(mesh, 0, mesh.vertices, GRID_SIZE * GRID_SIZE * 3 * sizeof(float), 0);
        }

        RebuildPhysicsShape(physics);
    }
}

void TerrainObject::RebuildPhysicsShape(Physics& physics)
{
    JPH::BodyInterface& bodyInterface = physics.system.GetBodyInterface();

    if (bodyId.IsInvalid() == false)
    {
        bodyInterface.RemoveBody(bodyId);
        bodyInterface.DestroyBody(bodyId);
        bodyId = JPH::BodyID();
    }

    float offsetX = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float offsetZ = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;

    JPH::HeightFieldShapeSettings settings(
        heightData.data(),
        JPH::Vec3(offsetX, 0.0f, offsetZ),
        JPH::Vec3(TERRAIN_SCALE, 1.0f, TERRAIN_SCALE),
        GRID_SIZE
    );

    JPH::ShapeSettings::ShapeResult shapeResult = settings.Create();
    if (shapeResult.IsValid())
    {
        JPH::BodyCreationSettings bodySettings(
            shapeResult.Get(),
            JPH::RVec3::sZero(),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::NON_MOVING
        );

        JPH::Body* body = bodyInterface.CreateBody(bodySettings);
        if (body)
        {
            bodyId = body->GetID();
            bodyInterface.AddBody(bodyId, JPH::EActivation::DontActivate);
        }
    }
}

void TerrainObject::PaintTextureAt(float worldX, float worldZ, int textureIndex, float strength, float radius)
{
    if (textureIndex < 0 || textureIndex > 3 || splatmapData.empty()) return;

    float offsetX = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float offsetZ = -(float)(GRID_SIZE - 1) * TERRAIN_SCALE * 0.5f;
    float terrainWidth = (float)(GRID_SIZE - 1) * TERRAIN_SCALE;

    float normX = (worldX - offsetX) / terrainWidth;
    float normZ = (worldZ - offsetZ) / terrainWidth;

    float centerPixelX = normX * (float)(SPLATMAP_RES - 1);
    float centerPixelZ = normZ * (float)(SPLATMAP_RES - 1);
    float pixelRadius = (radius / terrainWidth) * (float)SPLATMAP_RES;

    int minPX = std::max(0, (int)floorf(centerPixelX - pixelRadius));
    int maxPX = std::min(SPLATMAP_RES - 1, (int)ceilf(centerPixelX + pixelRadius));
    int minPZ = std::max(0, (int)floorf(centerPixelZ - pixelRadius));
    int maxPZ = std::min(SPLATMAP_RES - 1, (int)ceilf(centerPixelZ + pixelRadius));

    bool modified = false;

    for (int pz = minPZ; pz <= maxPZ; pz++)
    {
        for (int px = minPX; px <= maxPX; px++)
        {
            float dx = (float)px - centerPixelX;
            float dz = (float)pz - centerPixelZ;
            float dist = sqrtf(dx * dx + dz * dz);

            if (dist <= pixelRadius)
            {
                float falloff = cosf((dist / pixelRadius) * (3.14159265f * 0.5f));
                if (falloff <= 0.0f) continue;

                float blendFactor = std::clamp(strength * falloff * 5.0f, 0.0f, 1.0f);

                int idx = (pz * SPLATMAP_RES + px) * 4;
                float currentVal = (float)splatmapData[idx + textureIndex];
                float targetVal = 255.0f;
                float newVal = currentVal + (targetVal - currentVal) * blendFactor;

                float oldOtherSum = 0.0f;
                for (int c = 0; c < 4; c++)
                {
                    if (c != textureIndex) oldOtherSum += (float)splatmapData[idx + c];
                }

                splatmapData[idx + textureIndex] = (uint8_t)std::clamp(newVal, 0.0f, 255.0f);

                float remaining = 255.0f - (float)splatmapData[idx + textureIndex];
                if (oldOtherSum > 0.001f)
                {
                    for (int c = 0; c < 4; c++)
                    {
                        if (c != textureIndex)
                        {
                            float factor = (float)splatmapData[idx + c] / oldOtherSum;
                            splatmapData[idx + c] = (uint8_t)std::clamp(remaining * factor, 0.0f, 255.0f);
                        }
                    }
                }
                else
                {
                    for (int c = 0; c < 4; c++)
                    {
                        if (c != textureIndex) splatmapData[idx + c] = 0;
                    }
                }

                modified = true;
            }
        }
    }

    if (modified && splatmapTexture.id > 0)
    {
        UpdateTexture(splatmapTexture, splatmapData.data());
    }
}
