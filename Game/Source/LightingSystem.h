#ifndef LIGHTING_SYSTEM_H
#define LIGHTING_SYSTEM_H

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

struct LightingSystem
{
    Vector3 lightDir = { 0.35f, -0.85f, -0.35f };
    Vector4 lightColor = { 1.0f, 0.95f, 0.85f, 1.0f };
    Vector4 ambientColor = { 0.22f, 0.26f, 0.35f, 1.0f };

    Shader shadowShader = {};
    RenderTexture2D shadowMap = {};
    Camera3D lightCamera = {};

    Matrix lightViewProj = {};
    int shadowMapResolution = 2048;
    int shadowMapSlot = 10;

    int locLightDir = -1;
    int locLightColor = -1;
    int locAmbient = -1;
    int locViewPos = -1;
    int locLightVP = -1;
    int locShadowMap = -1;
    int locShadowMapRes = -1;

    bool isLoaded = false;

    void Init(int mapResolution = 2048);
    void BeginShadowMapPass(Vector3 focusPosition);
    void EndShadowMapPass();
    void BindLightingUniforms(Vector3 cameraPos);
    void ApplyToModel(Model& model);
    void Shutdown();
};

#endif // LIGHTING_SYSTEM_H
