#include "Skybox.h"
#include "rlgl.h"
#include "raymath.h"
#include <cstdio>

static Shader TryLoadSkyShader()
{
    const char* vsPaths[] = {
        "Shaders/skybox.vs",
        "Assets/Shaders/skybox.vs",
        "Build/Development/Assets/Shaders/skybox.vs"
    };

    const char* fsPaths[] = {
        "Shaders/skybox.fs",
        "Assets/Shaders/skybox.fs",
        "Build/Development/Assets/Shaders/skybox.fs"
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
    return (Shader){ 0 };
}

void Skybox::Init()
{
    skyShader = TryLoadSkyShader();
    if (skyShader.id > 0)
    {
        lightDirLoc = GetShaderLocation(skyShader, "lightDir");

        skyShader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(skyShader, "matProjection");
        skyShader.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(skyShader, "matView");

        cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        skyModel = LoadModelFromMesh(cubeMesh);
        skyModel.materials[0].shader = skyShader;

        isLoaded = true;
        TraceLog(LOG_INFO, "[Skybox] Procedural Skybox inicializado com sucesso.");
    }
    else
    {
        TraceLog(LOG_WARNING, "[Skybox] Não foi possível carregar os shaders do skybox.");
    }
}

void Skybox::Draw(Camera camera, Vector3 lightDir)
{
    if (!isLoaded)
    {
        ClearBackground(SKYBLUE);
        return;
    }

    // Passa a direção da luz solar para o shader do skybox
    Vector3 normLightDir = Vector3Normalize(lightDir);
    SetShaderValue(skyShader, lightDirLoc, &normLightDir, SHADER_UNIFORM_VEC3);

    // Desabilita a gravação no buffer de profundidade e culling para desenhar o interior do cubo no infinito
    rlDisableDepthMask();
    rlDisableDepthTest();
    rlDisableBackfaceCulling();

    BeginMode3D(camera);
        // Desenha o cubo do skybox centralizado no infinito
        DrawModel(skyModel, (Vector3){0.0f, 0.0f, 0.0f}, 100.0f, WHITE);
    EndMode3D();

    rlEnableBackfaceCulling();
    rlEnableDepthTest();
    rlEnableDepthMask();
}

void Skybox::Shutdown()
{
    if (!isLoaded) return;

    UnloadModel(skyModel);
    UnloadShader(skyShader);
    skyModel = (Model){ 0 };
    skyShader = (Shader){ 0 };
    isLoaded = false;
}
