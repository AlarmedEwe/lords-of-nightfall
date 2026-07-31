#include "TerrainObject.h"
#include "rlgl.h"
#include <cmath>
#include <cstdlib>

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

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);
    meshGenerated = true;
}

void TerrainObject::Init(Physics& physics)
{
    GenerateHeightmap();
    GenerateMesh();

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
    DrawModel(model, position, 1.0f, DARKGREEN);
    DrawModelWires(model, position, 1.0f, (Color){30, 60, 30, 80});
}

void TerrainObject::Shutdown(Physics& physics)
{
    JPH::BodyInterface& bodyInterface = physics.system.GetBodyInterface();
    bodyInterface.RemoveBody(bodyId);
    bodyInterface.DestroyBody(bodyId);

    if (meshGenerated)
    {
        UnloadModel(model);
        meshGenerated = false;
    }
}

float TerrainObject::GetHeightAt(float x, float z)
{
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
