#include "LONModelIO.h"
#include "LONModel.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <unordered_map>
#include <string>

namespace LONModelIO
{

static std::unordered_map<std::string, Texture2D> s_TextureCache;

static Texture2D TryLoadTexture(const char* relPath)
{
    if (!relPath || strlen(relPath) == 0) return (Texture2D){ 0 };

    std::string key = relPath;
    auto it = s_TextureCache.find(key);
    if (it != s_TextureCache.end() && it->second.id > 0)
    {
        return it->second;
    }

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
            {
                s_TextureCache[key] = tex;
                return tex;
            }
        }
    }
    return (Texture2D){ 0 };
}

void ClearTextureCache()
{
    for (auto& pair : s_TextureCache)
    {
        if (pair.second.id > 0)
        {
            UnloadTexture(pair.second);
        }
    }
    s_TextureCache.clear();
}

bool Load(LONModel& model, const char* lonmodelPath, const char* texturePath)
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
        TraceLog(LOG_WARNING, "[LONModel] Arquivo nao encontrado: %s", lonmodelPath);
        return false;
    }

    char magic[4] = {0};
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "LONM", 4) != 0)
    {
        TraceLog(LOG_ERROR, "[LONModel] Magic de arquivo invalido em %s", lonmodelPath);
        fclose(f);
        return false;
    }

    uint32_t version = 0;
    uint32_t numMeshes = 0;
    uint32_t numAnims = 0;

    if (fread(&version, sizeof(uint32_t), 1, f) != 1 ||
        fread(&numMeshes, sizeof(uint32_t), 1, f) != 1 ||
        fread(&numAnims, sizeof(uint32_t), 1, f) != 1)
    {
        TraceLog(LOG_ERROR, "[LONModel] Erro ao ler cabecalho do arquivo %s", lonmodelPath);
        fclose(f);
        return false;
    }

    model.animClips.resize(numAnims);
    model.handFrames.resize(numAnims);

    for (uint32_t a = 0; a < numAnims; a++)
    {
        char nameBuf[32] = {0};
        uint32_t fCount = 0;
        float fps = 30.0f;
        if (fread(nameBuf, 1, 32, f) == 32 &&
            fread(&fCount, sizeof(uint32_t), 1, f) == 1 &&
            fread(&fps, sizeof(float), 1, f) == 1)
        {
            model.animClips[a].name = nameBuf;
            model.animClips[a].frameCount = fCount;
            model.animClips[a].fps = fps;
        }

        if (version >= 3)
        {
            model.handFrames[a].resize(fCount);
            for (uint32_t fr = 0; fr < fCount; fr++)
            {
                fread(&model.handFrames[a][fr], sizeof(float), 16, f);
            }
        }
    }

    model.rayModel = (Model){ 0 };
    model.rayModel.meshCount = numMeshes;
    model.rayModel.meshes = (Mesh*)RL_MALLOC(numMeshes * sizeof(Mesh));
    model.rayModel.transform = MatrixIdentity();

    if (version >= 4)
    {
        model.rayModel.materialCount = numMeshes;
        model.rayModel.materials = (Material*)RL_MALLOC(numMeshes * sizeof(Material));
        model.rayModel.meshMaterial = (int*)RL_MALLOC(numMeshes * sizeof(int));
    }
    else
    {
        model.rayModel.materialCount = 1;
        model.rayModel.materials = (Material*)RL_MALLOC(sizeof(Material));
        model.rayModel.materials[0] = LoadMaterialDefault();
        model.rayModel.meshMaterial = (int*)RL_MALLOC(numMeshes * sizeof(int));

        model.texture = TryLoadTexture(texturePath);
        if (model.texture.id > 0)
        {
            SetMaterialTexture(&model.rayModel.materials[0], MATERIAL_MAP_DIFFUSE, model.texture);
        }
    }

    model.submeshAnims.resize(numMeshes);

    for (uint32_t i = 0; i < numMeshes; i++)
    {
        if (version >= 4)
        {
            char submeshTexName[64] = {0};
            if (fread(submeshTexName, 1, 64, f) != 64)
            {
                break;
            }

            model.rayModel.meshMaterial[i] = (int)i;
            model.rayModel.materials[i] = LoadMaterialDefault();

            if (strlen(submeshTexName) > 0)
            {
                Texture2D tex = TryLoadTexture(submeshTexName);
                if (tex.id > 0)
                {
                    SetMaterialTexture(&model.rayModel.materials[i], MATERIAL_MAP_DIFFUSE, tex);
                }
            }
        }
        else
        {
            model.rayModel.meshMaterial[i] = 0;
        }

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

        model.submeshAnims[i].vertexFrames.resize(numAnims);
        for (uint32_t a = 0; a < numAnims; a++)
        {
            uint32_t fCount = model.animClips[a].frameCount;
            model.submeshAnims[i].vertexFrames[a].resize(fCount);
            for (uint32_t fr = 0; fr < fCount; fr++)
            {
                model.submeshAnims[i].vertexFrames[a][fr].resize(numVerts * 3);
                fread(model.submeshAnims[i].vertexFrames[a][fr].data(), sizeof(float), numVerts * 3, f);
            }
        }

        if (!model.submeshAnims[i].vertexFrames.empty() && !model.submeshAnims[i].vertexFrames[0].empty())
        {
            memcpy(mesh.vertices, model.submeshAnims[i].vertexFrames[0][0].data(), numVerts * 3 * sizeof(float));
        }

        UploadMesh(&mesh, true);
        model.rayModel.meshes[i] = mesh;
    }

    fclose(f);
    model.isLoaded = true;
    model.currentAnimIndex = 0;
    model.prevAnimIndex = 0;
    model.animTime = 0.0f;
    model.prevAnimTime = 0.0f;
    model.isBlending = false;

    TraceLog(LOG_INFO, "[LONModel] Modelo v%d %s carregado com sucesso (%d submeshes, %d clipes de animacao)",
             version, lonmodelPath, numMeshes, numAnims);
    return true;
}

void Unload(LONModel& model)
{
    if (!model.isLoaded) return;

    model.texture = (Texture2D){ 0 };

    UnloadModel(model.rayModel);
    model.rayModel = (Model){ 0 };
    model.submeshAnims.clear();
    model.animClips.clear();
    model.handFrames.clear();
    model.currentRightHandMatrix = MatrixIdentity();
    model.isLoaded = false;
}

} // namespace LONModelIO
