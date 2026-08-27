#include "Scene.h"
#include "GameObjects/PlayerObject.h"
#include "GameObjects/TerrainObject.h"
#include "GameObjects/NatureObject.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

void Scene::Init()
{
    physics.Init();

    camera.position = {0.0f, 10.0f, -10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Inicializa o Skybox e o Sistema de Iluminação/Sombras
    skybox.Init();
    lightingSystem.Init(2048);

    for (auto* obj : objects)
    {
        obj->Init(physics);

        if (auto* terrain = dynamic_cast<TerrainObject*>(obj))
        {
            lightingSystem.ApplyToModel(terrain->model);
        }
        else if (auto* player = dynamic_cast<PlayerObject*>(obj))
        {
            if (player->characterModel.isLoaded)
            {
                lightingSystem.ApplyToModel(player->characterModel.rayModel);
            }
        }
        else if (auto* nature = dynamic_cast<NatureObject*>(obj))
        {
            if (nature->natureModel && nature->natureModel->isLoaded)
            {
                lightingSystem.ApplyToModel(nature->natureModel->rayModel);
            }
        }
    }

    physics.system.OptimizeBroadPhase();
}

void Scene::Update(float deltaTime)
{
    physics.Update(deltaTime);
    damageFeedback.Update(deltaTime);

    for (auto* obj : objects)
        obj->Update(physics, deltaTime);
}

void Scene::Draw()
{
    Vector3 focusPos = {0.0f, 0.0f, 0.0f};
    for (auto* obj : objects)
    {
        if (auto* player = dynamic_cast<PlayerObject*>(obj))
        {
            focusPos = player->position;
            if (player->characterModel.isLoaded && player->characterModel.rayModel.materials[0].shader.id == 0)
            {
                lightingSystem.ApplyToModel(player->characterModel.rayModel);
            }
            break;
        }
    }

    // PASS 1: Renderiza o mapa de sombras a partir do ponto de vista do Sol
    lightingSystem.BeginShadowMapPass(focusPos);
    for (auto* obj : objects)
    {
        obj->Draw();
    }
    lightingSystem.EndShadowMapPass();

    // PASS 2: Desenha a cena no framebuffer principal com Skybox e Sombras
    skybox.Draw(camera, lightingSystem.lightDir);

    lightingSystem.BindLightingUniforms(camera.position);

    BeginMode3D(camera);
    for (auto* obj : objects)
    {
        obj->Draw();
    }
    damageFeedback.DrawParticles();
    EndMode3D();
}

#include "Model/LONModelIO.h"

void Scene::Shutdown()
{
    damageFeedback.Clear();
    skybox.Shutdown();
    lightingSystem.Shutdown();

    for (auto* obj : objects)
        obj->Shutdown(physics);

    NatureObject::ClearCache();
    LONModelIO::ClearTextureCache();

    physics.Shutdown();
}

void Scene::UpdateCamera(Vector3 playerPosition, float deltaTime, float sensitivity)
{
    (void)deltaTime;

    // Mouse input para rotação com sensibilidade aplicada
    Vector2 mouseDelta = GetMouseDelta();
    cameraYaw -= mouseDelta.x * 0.3f * sensitivity;
    cameraPitch += mouseDelta.y * 0.3f * sensitivity;

    // Clamp pitch
    if (cameraPitch < -85.0f) cameraPitch = -85.0f;
    if (cameraPitch > 85.0f) cameraPitch = 85.0f;

    // Scroll para zoom
    float wheel = GetMouseWheelMove();
    cameraDistance -= wheel * 1.5f;
    cameraDistance = std::clamp(cameraDistance, cameraMinDistance, cameraMaxDistance);

    // Calcula posição da câmera baseado em yaw/pitch/distance
    float yawRad = cameraYaw * DEG2RAD;
    float pitchRad = cameraPitch * DEG2RAD;

    // Vetor lateral da câmera para offset do ombro
    Vector3 right = { cosf(yawRad), 0.0f, -sinf(yawRad) };

    // Câmera estilo Over-The-Shoulder: Target posicionado no ombro do personagem
    cameraTarget = {
        playerPosition.x + right.x * 0.5f,
        playerPosition.y + 0.6f, // Alinhado com o peito/ombros
        playerPosition.z + right.z * 0.5f
    };

    Vector3 offset;
    offset.x = cosf(pitchRad) * sinf(yawRad) * cameraDistance;
    offset.y = sinf(pitchRad) * cameraDistance;
    offset.z = cosf(pitchRad) * cosf(yawRad) * cameraDistance;

    Vector3 desiredPosition = {
        cameraTarget.x + offset.x,
        cameraTarget.y + offset.y,
        cameraTarget.z + offset.z
    };

    // Colisão de câmera: raycast do target até a posição desejada
    JPH::Vec3 rayOrigin(cameraTarget.x, cameraTarget.y, cameraTarget.z);
    JPH::Vec3 rayDir(offset.x, offset.y, offset.z);
    float rayLength = cameraDistance;

    if (rayLength > 0.1f)
    {
        JPH::Vec3 normalizedDir = rayDir.Normalized();
        JPH::RVec3 hitPoint;
        if (physics.CastRay(rayOrigin, normalizedDir, rayLength, hitPoint))
        {
            Vector3 hitPos = {(float)hitPoint.GetX(), (float)hitPoint.GetY(), (float)hitPoint.GetZ()};
            Vector3 diff = {hitPos.x - cameraTarget.x, hitPos.y - cameraTarget.y, hitPos.z - cameraTarget.z};
            float hitDist = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

            if (hitDist < cameraDistance)
            {
                float safeDistance = hitDist - 0.3f;
                if (safeDistance < cameraMinDistance) safeDistance = cameraMinDistance;

                float ratio = safeDistance / cameraDistance;
                desiredPosition = {
                    cameraTarget.x + offset.x * ratio,
                    cameraTarget.y + offset.y * ratio,
                    cameraTarget.z + offset.z * ratio
                };
            }
        }
    }

    camera.position = desiredPosition;
    camera.target = cameraTarget;
}
