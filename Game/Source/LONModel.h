#ifndef LON_MODEL_H
#define LON_MODEL_H

#include "raylib.h"
#include <cstdint>
#include <vector>
#include <string>

struct LONAnimClip
{
    std::string name;
    uint32_t frameCount = 0;
    float fps = 30.0f;
};

struct LONSubmeshAnimFrames
{
    // vertexFrames[clipIndex][frameIndex] -> std::vector<float> (numVerts * 3)
    std::vector<std::vector<std::vector<float>>> vertexFrames;
};

struct LONModel
{
    Model rayModel = {};
    Texture2D texture = {};

    std::vector<LONAnimClip> animClips;
    std::vector<LONSubmeshAnimFrames> submeshAnims;

    int currentAnimIndex = 0;
    int prevAnimIndex = 0;
    float animTime = 0.0f;
    float prevAnimTime = 0.0f;

    bool isBlending = false;
    float blendTimer = 0.0f;
    float blendDuration = 0.18f; // 180ms smooth animation cross-fade

    bool isLoaded = false;

    void Load(const char* modelPath, const char* texturePath);
    void SetAnimation(int animIndex);
    void Update(float deltaTime);
    void Draw(Vector3 position, float yaw, float scale = 1.0f, Color tint = WHITE);
    void Unload();
};

#endif // LON_MODEL_H
