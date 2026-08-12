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
    target.texture.width = width;
    target.texture.height = height;

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
        shadowShader.locs[SHADER_LOC_MAP_SPECULAR] = locShadowMap;

        shadowMap = CreateShadowMapFBO(shadowMapResolution, shadowMapResolution);

        // Câmera ortográfica para iluminação direcional (Sol)
        lightCamera = (Camera3D){ 0 };
        lightCamera.projection = CAMERA_ORTHOGRAPHIC;
        lightCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        lightCamera.fovy = 50.0f; // Frustum ortográfico (50m x 50m) cobrindo área ampla ao redor do personagem

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
    // Posiciona a fonte de luz 50m acima do foco
    lightCamera.position = Vector3Subtract(focusPosition, Vector3Scale(normLightDir, 50.0f));
    lightCamera.target = focusPosition;

    Matrix lightView = MatrixLookAt(lightCamera.position, lightCamera.target, lightCamera.up);
    float size = lightCamera.fovy * 0.5f;
    Matrix lightProj = MatrixOrtho(-size, size, -size, size, 0.1f, 100.0f);
    lightViewProj = MatrixMultiply(lightView, lightProj);

    BeginTextureMode(shadowMap);
    ClearBackground(WHITE);

    BeginMode3D(lightCamera);
    rlSetCullFace(RL_CULL_FACE_FRONT); // Front-face culling durante pass de sombras elimina self-shadowing no terreno
}

void LightingSystem::EndShadowMapPass()
{
    if (!isLoaded) return;

    EndMode3D();
    rlSetCullFace(RL_CULL_FACE_BACK); // Restaura back-face culling padrão
    EndTextureMode();
}

void LightingSystem::BindLightingUniforms(Vector3 cameraPos)
{
    if (!isLoaded) return;

    Vector3 normLightDir = Vector3Normalize(lightDir);
    int slot1 = 1;

    SetShaderValue(shadowShader, locLightDir, &normLightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shadowShader, locLightColor, &lightColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(shadowShader, locAmbient, &ambientColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(shadowShader, locViewPos, &cameraPos, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(shadowShader, locLightVP, lightViewProj);
    SetShaderValue(shadowShader, locShadowMapRes, &shadowMapResolution, SHADER_UNIFORM_INT);
    SetShaderValue(shadowShader, locShadowMap, &slot1, SHADER_UNIFORM_INT);
}

void LightingSystem::ApplyToModel(Model& model)
{
    if (!isLoaded) return;

    for (int i = 0; i < model.materialCount; i++)
    {
        model.materials[i].shader = shadowShader;
        model.materials[i].maps[1].texture = shadowMap.depth;
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
