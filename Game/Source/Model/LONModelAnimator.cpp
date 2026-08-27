#include "LONModelAnimator.h"
#include "LONModel.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace LONModelAnimator
{

void SetAnimation(LONModel& model, int animIndex)
{
    if (!model.isLoaded || model.animClips.empty()) return;
    if (animIndex < 0 || animIndex >= (int)model.animClips.size()) animIndex = 0;

    if (model.currentAnimIndex != animIndex)
    {
        model.prevAnimIndex = model.currentAnimIndex;
        model.prevAnimTime = model.animTime;

        model.currentAnimIndex = animIndex;
        model.animTime = 0.0f;

        model.isBlending = true;
        model.blendTimer = 0.0f;
    }
}

void Update(LONModel& model, float deltaTime)
{
    if (!model.isLoaded || model.animClips.empty()) return;

    const LONAnimClip& clip = model.animClips[model.currentAnimIndex];
    if (clip.frameCount == 0) return;

    model.animTime += deltaTime * clip.fps;
    float currentFrameF = fmodf(model.animTime, (float)clip.frameCount);
    uint32_t frameIdx = (uint32_t)currentFrameF;

    float blendFactor = 1.0f;
    uint32_t prevFrameIdx = 0;
    const LONAnimClip* prevClip = nullptr;

    if (model.isBlending)
    {
        model.blendTimer += deltaTime;
        blendFactor = std::min(1.0f, model.blendTimer / model.blendDuration);

        if (blendFactor >= 1.0f)
        {
            model.isBlending = false;
        }
        else if (model.prevAnimIndex >= 0 && model.prevAnimIndex < (int)model.animClips.size())
        {
            prevClip = &model.animClips[model.prevAnimIndex];
            model.prevAnimTime += deltaTime * prevClip->fps;
            if (prevClip->frameCount > 0)
            {
                prevFrameIdx = (uint32_t)fmodf(model.prevAnimTime, (float)prevClip->frameCount);
            }
        }
    }

    // Atualiza matriz da mao direita (handFrames)
    if (!model.handFrames.empty() && model.currentAnimIndex < (int)model.handFrames.size())
    {
        const auto& currHandClips = model.handFrames[model.currentAnimIndex];
        if (frameIdx < currHandClips.size())
        {
            Matrix currMat = currHandClips[frameIdx];

            if (model.isBlending && prevClip && model.prevAnimIndex < (int)model.handFrames.size())
            {
                const auto& prevHandClips = model.handFrames[model.prevAnimIndex];
                if (prevFrameIdx < prevHandClips.size())
                {
                    Matrix prevMat = prevHandClips[prevFrameIdx];

                    Vector3 pPos = { prevMat.m12, prevMat.m13, prevMat.m14 };
                    Vector3 cPos = { currMat.m12, currMat.m13, currMat.m14 };
                    Vector3 bPos = Vector3Lerp(pPos, cPos, blendFactor);

                    Quaternion pRot = QuaternionFromMatrix(prevMat);
                    Quaternion cRot = QuaternionFromMatrix(currMat);
                    Quaternion bRot = QuaternionSlerp(pRot, cRot, blendFactor);

                    model.currentRightHandMatrix = MatrixMultiply(QuaternionToMatrix(bRot), MatrixTranslate(bPos.x, bPos.y, bPos.z));
                }
                else
                {
                    model.currentRightHandMatrix = currMat;
                }
            }
            else
            {
                model.currentRightHandMatrix = currMat;
            }
        }
    }

    for (int i = 0; i < model.rayModel.meshCount; i++)
    {
        Mesh& mesh = model.rayModel.meshes[i];
        if (i < (int)model.submeshAnims.size() && model.currentAnimIndex < (int)model.submeshAnims[i].vertexFrames.size())
        {
            const auto& currFrames = model.submeshAnims[i].vertexFrames[model.currentAnimIndex];

            if (model.isBlending && prevClip && model.prevAnimIndex < (int)model.submeshAnims[i].vertexFrames.size())
            {
                const auto& prevFrames = model.submeshAnims[i].vertexFrames[model.prevAnimIndex];
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

} // namespace LONModelAnimator
