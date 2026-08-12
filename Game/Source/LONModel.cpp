#include "LONModel.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

static Texture2D TryLoadTexture(const char* relPath)
{
    const char* candidatePaths[] = {
        relPath,
        TextFormat("Assets/%s", relPath),
        TextFormat("Assets/Textures/%s", relPath),
        TextFormat("Build/Development/Assets/%s", relPath),
        TextFormat("Build/Development/Assets/Textures/%s", relPath),
        TextFormat("Game/Assets/Universal Base Characters/Base Characters/Textures/%s", relPath)
    };

    for (const char* path : candidatePaths)
    {
        if (FileExists(path))
        {
            Texture2D tex = LoadTexture(path);
            if (tex.id > 0)
                return tex;
        }
    }
    return (Texture2D){ 0 };
}

void LONModel::Load(const char* lonmodelPath, const char* texturePath)
{
    const char* candidatePaths[] = {
        lonmodelPath,
        TextFormat("Assets/Models/%s", lonmodelPath),
        TextFormat("Build/Development/Assets/Models/%s", lonmodelPath),
        TextFormat("Assets/%s", lonmodelPath),
        TextFormat("Build/Development/Assets/%s", lonmodelPath)
    };

    FILE* f = nullptr;
    for (const char* path : candidatePaths)
    {
        if (FileExists(path))
        {
            f = fopen(path, "rb");
            if (f) break;
        }
    }

    if (!f)
    {
        TraceLog(LOG_WARNING, "[LONModel] Arquivo não encontrado: %s", lonmodelPath);
        return;
    }

    char magic[4] = {0};
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "LONM", 4) != 0)
    {
        TraceLog(LOG_ERROR, "[LONModel] Magic de arquivo inválido em %s", lonmodelPath);
        fclose(f);
        return;
    }

    uint32_t version = 0;
    uint32_t numMeshes = 0;
    uint32_t numAnims = 0;

    if (fread(&version, sizeof(uint32_t), 1, f) != 1 ||
        fread(&numMeshes, sizeof(uint32_t), 1, f) != 1 ||
        fread(&numAnims, sizeof(uint32_t), 1, f) != 1)
    {
        TraceLog(LOG_ERROR, "[LONModel] Erro ao ler cabeçalho do arquivo %s", lonmodelPath);
        fclose(f);
        return;
    }

    animClips.resize(numAnims);
    for (uint32_t a = 0; a < numAnims; a++)
    {
        char nameBuf[32] = {0};
        uint32_t fCount = 0;
        float fps = 30.0f;
        if (fread(nameBuf, 1, 32, f) == 32 &&
            fread(&fCount, sizeof(uint32_t), 1, f) == 1 &&
            fread(&fps, sizeof(float), 1, f) == 1)
        {
            animClips[a].name = nameBuf;
            animClips[a].frameCount = fCount;
            animClips[a].fps = fps;
        }
    }

    rayModel = (Model){ 0 };
    rayModel.meshCount = numMeshes;
    rayModel.meshes = (Mesh*)RL_MALLOC(numMeshes * sizeof(Mesh));
    rayModel.materialCount = 1;
    rayModel.materials = (Material*)RL_MALLOC(sizeof(Material));
    rayModel.materials[0] = LoadMaterialDefault();
    rayModel.meshMaterial = (int*)RL_MALLOC(numMeshes * sizeof(int));
    rayModel.transform = MatrixIdentity();

    // Carrega a textura do personagem
    texture = TryLoadTexture(texturePath);
    if (texture.id > 0)
    {
        SetMaterialTexture(&rayModel.materials[0], MATERIAL_MAP_DIFFUSE, texture);
    }

    submeshAnims.resize(numMeshes);

    for (uint32_t i = 0; i < numMeshes; i++)
    {
        rayModel.meshMaterial[i] = 0;

        uint32_t numVerts = 0;
        uint32_t numIndices = 0;
        if (fread(&numVerts, sizeof(uint32_t), 1, f) != 1 ||
            fread(&numIndices, sizeof(uint32_t), 1, f) != 1)
        {
            break;
        }

        Mesh mesh = {};
        mesh.vertexCount = (int)numVerts;
        mesh.triangleCount = (int)numIndices / 3;

        mesh.vertices = (float*)RL_MALLOC(numVerts * 3 * sizeof(float));
        mesh.normals = (float*)RL_MALLOC(numVerts * 3 * sizeof(float));
        mesh.texcoords = (float*)RL_MALLOC(numVerts * 2 * sizeof(float));
        mesh.indices = (unsigned short*)RL_MALLOC(numIndices * sizeof(unsigned short));

        if (fread(mesh.normals, sizeof(float), numVerts * 3, f) != numVerts * 3 ||
            fread(mesh.texcoords, sizeof(float), numVerts * 2, f) != numVerts * 2)
        {
            TraceLog(LOG_ERROR, "[LONModel] Erro ao ler normais/UVs da submesh %u", i);
        }

        std::vector<uint32_t> tempIndices(numIndices);
        if (fread(tempIndices.data(), sizeof(uint32_t), numIndices, f) == numIndices)
        {
            for (uint32_t idx = 0; idx < numIndices; idx++)
            {
                mesh.indices[idx] = (unsigned short)tempIndices[idx];
            }
        }

        submeshAnims[i].vertexFrames.resize(numAnims);
        for (uint32_t a = 0; a < numAnims; a++)
        {
            uint32_t fCount = animClips[a].frameCount;
            submeshAnims[i].vertexFrames[a].resize(fCount);
            for (uint32_t fr = 0; fr < fCount; fr++)
            {
                submeshAnims[i].vertexFrames[a][fr].resize(numVerts * 3);
                fread(submeshAnims[i].vertexFrames[a][fr].data(), sizeof(float), numVerts * 3, f);
            }
        }

        // Define a pose inicial do frame 0 do primeiro clipe
        if (!submeshAnims[i].vertexFrames.empty() && !submeshAnims[i].vertexFrames[0].empty())
        {
            memcpy(mesh.vertices, submeshAnims[i].vertexFrames[0][0].data(), numVerts * 3 * sizeof(float));
        }

        UploadMesh(&mesh, true); // Dynamic mesh VBO update
        rayModel.meshes[i] = mesh;
    }

    fclose(f);
    isLoaded = true;
    currentAnimIndex = 0;
    prevAnimIndex = 0;
    animTime = 0.0f;
    prevAnimTime = 0.0f;
    isBlending = false;

    TraceLog(LOG_INFO, "[LONModel] Modelo v2 %s carregado com sucesso (%d submeshes, %d clipes de animação)",
             lonmodelPath, numMeshes, numAnims);
}

void LONModel::SetAnimation(int animIndex)
{
    if (!isLoaded || animClips.empty()) return;
    if (animIndex < 0 || animIndex >= (int)animClips.size()) animIndex = 0;

    if (currentAnimIndex != animIndex)
    {
        prevAnimIndex = currentAnimIndex;
        prevAnimTime = animTime;

        currentAnimIndex = animIndex;
        animTime = 0.0f;

        isBlending = true;
        blendTimer = 0.0f;
    }
}

void LONModel::Update(float deltaTime)
{
    if (!isLoaded || animClips.empty()) return;

    const LONAnimClip& clip = animClips[currentAnimIndex];
    if (clip.frameCount == 0) return;

    animTime += deltaTime * clip.fps;
    float currentFrameF = fmodf(animTime, (float)clip.frameCount);
    uint32_t frameIdx = (uint32_t)currentFrameF;

    float blendFactor = 1.0f;
    uint32_t prevFrameIdx = 0;
    const LONAnimClip* prevClip = nullptr;

    if (isBlending)
    {
        blendTimer += deltaTime;
        blendFactor = std::min(1.0f, blendTimer / blendDuration);

        if (blendFactor >= 1.0f)
        {
            isBlending = false;
        }
        else if (prevAnimIndex >= 0 && prevAnimIndex < (int)animClips.size())
        {
            prevClip = &animClips[prevAnimIndex];
            prevAnimTime += deltaTime * prevClip->fps;
            if (prevClip->frameCount > 0)
            {
                prevFrameIdx = (uint32_t)fmodf(prevAnimTime, (float)prevClip->frameCount);
            }
        }
    }

    for (int i = 0; i < rayModel.meshCount; i++)
    {
        Mesh& mesh = rayModel.meshes[i];
        if (i < (int)submeshAnims.size() && currentAnimIndex < (int)submeshAnims[i].vertexFrames.size())
        {
            const auto& currFrames = submeshAnims[i].vertexFrames[currentAnimIndex];

            if (isBlending && prevClip && prevAnimIndex < (int)submeshAnims[i].vertexFrames.size())
            {
                const auto& prevFrames = submeshAnims[i].vertexFrames[prevAnimIndex];
                if (frameIdx < currFrames.size() && prevFrameIdx < prevFrames.size())
                {
                    const float* pVerts = prevFrames[prevFrameIdx].data();
                    const float* cVerts = currFrames[frameIdx].data();
                    int numFloats = mesh.vertexCount * 3;

                    for (int k = 0; k < numFloats; k++)
                    {
                        mesh.vertices[k] = pVerts[k] + (cVerts[k] - pVerts[k]) * blendFactor;
                    }
                    UpdateMeshBuffer(mesh, 0, mesh.vertices, numFloats * sizeof(float), 0);
                }
            }
            else if (frameIdx < currFrames.size())
            {
                memcpy(mesh.vertices, currFrames[frameIdx].data(), mesh.vertexCount * 3 * sizeof(float));
                UpdateMeshBuffer(mesh, 0, mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);
            }
        }
    }
}

void LONModel::Draw(Vector3 position, float yaw, float scale, Color tint)
{
    if (!isLoaded || rayModel.meshCount == 0) return;

    Vector3 axis = { 0.0f, 1.0f, 0.0f };
    DrawModelEx(rayModel, position, axis, yaw, (Vector3){ scale, scale, scale }, tint);
}

void LONModel::Unload()
{
    if (!isLoaded) return;

    if (texture.id > 0)
    {
        UnloadTexture(texture);
        texture = (Texture2D){ 0 };
    }

    UnloadModel(rayModel);
    rayModel = (Model){ 0 };
    submeshAnims.clear();
    animClips.clear();
    isLoaded = false;
}
