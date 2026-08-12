#include "LightingSystem.h"
#include <cstdio>

static Shader TryLoadLightingShader()
{
    const char* vsPaths[] = {
        "Shaders/lighting.vs",
        "Assets/Shaders/lighting.vs",
        "Build/Development/Assets/Shaders/lighting.vs"
    };

    const char* fsPaths[] = {
        "Shaders/lighting.fs",
        "Assets/Shaders/lighting.fs",
        "Build/Development/Assets/Shaders/lighting.fs"
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

static RenderTexture2D CreateShadowMapFBO(int width, int height)
{
    RenderTexture2D target = { 0 };
    target.id = rlLoadFramebuffer();

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;
        target.depth.mipmaps = 1;

        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        if (rlFramebufferComplete(target.id))
        {
            TraceLog(LOG_INFO, "[LightingSystem] ShadowMap FBO %dx%d criado com sucesso (ID: %d)", width, height, target.id);
        }

        rlDisableFramebuffer();
    }
    return target;
}

void LightingSystem::Init(int mapResolution)
{
    shadowMapResolution = mapResolution;
    shadowShader = TryLoadLightingShader();

    if (shadowShader.id > 0)
    {
        locLightDir = GetShaderLocation(shadowShader, "lightDir");
        locLightColor = GetShaderLocation(shadowShader, "lightColor");
        locAmbient = GetShaderLocation(shadowShader, "ambient");
        locViewPos = GetShaderLocation(shadowShader, "viewPos");
        locLightVP = GetShaderLocation(shadowShader, "lightVP");
        locShadowMap = GetShaderLocation(shadowShader, "shadowMap");
        locShadowMapRes = GetShaderLocation(shadowShader, "shadowMapResolution");
        shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = locViewPos;

        shadowMap = CreateShadowMapFBO(shadowMapResolution, shadowMapResolution);

        // Câmera ortográfica para luz solar direcional
        lightCamera = (Camera3D){ 0 };
        lightCamera.projection = CAMERA_ORTHOGRAPHIC;
        lightCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        lightCamera.fovy = 50.0f; // Tamanho do volume ortográfico que cobre o personagem e terreno próximo

        isLoaded = true;
        TraceLog(LOG_INFO, "[LightingSystem] Sistema de Iluminação e Sombras inicializado com sucesso.");
    }
    else
    {
        TraceLog(LOG_WARNING, "[LightingSystem] Não foi possível carregar os shaders de iluminação/sombras.");
    }
}

void LightingSystem::BeginShadowMapPass(Vector3 focusPosition)
{
    if (!isLoaded) return;

    Vector3 normLightDir = Vector3Normalize(lightDir);
    lightCamera.position = Vector3Subtract(focusPosition, Vector3Scale(normLightDir, 40.0f));
    lightCamera.target = focusPosition;

    BeginTextureMode(shadowMap);
    ClearBackground(WHITE);

    BeginMode3D(lightCamera);
}

void LightingSystem::EndShadowMapPass()
{
    if (!isLoaded) return;

    Matrix lightView = rlGetMatrixModelview();
    Matrix lightProj = rlGetMatrixProjection();
    lightViewProj = MatrixMultiply(lightView, lightProj);

    EndMode3D();
    EndTextureMode();
}

void LightingSystem::BindLightingUniforms(Vector3 cameraPos)
{
    if (!isLoaded) return;

    Vector3 normLightDir = Vector3Normalize(lightDir);

    SetShaderValue(shadowShader, locLightDir, &normLightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shadowShader, locLightColor, &lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(shadowShader, locAmbient, &ambientColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(shadowShader, locViewPos, &cameraPos, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(shadowShader, locLightVP, lightViewProj);
    SetShaderValue(shadowShader, locShadowMapRes, &shadowMapResolution, SHADER_UNIFORM_INT);

    rlEnableShader(shadowShader.id);
    rlActiveTextureSlot(shadowMapSlot);
    rlEnableTexture(shadowMap.depth.id);
    rlSetUniform(locShadowMap, &shadowMapSlot, SHADER_UNIFORM_INT, 1);
}

void LightingSystem::ApplyToModel(Model& model)
{
    if (!isLoaded) return;

    for (int i = 0; i < model.materialCount; i++)
    {
        model.materials[i].shader = shadowShader;
    }
}

void LightingSystem::Shutdown()
{
    if (!isLoaded) return;

    if (shadowMap.id > 0)
    {
        rlUnloadFramebuffer(shadowMap.id);
        shadowMap = (RenderTexture2D){ 0 };
    }

    if (shadowShader.id > 0)
    {
        UnloadShader(shadowShader);
        shadowShader = (Shader){ 0 };
    }

    isLoaded = false;
}
