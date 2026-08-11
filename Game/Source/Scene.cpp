#include "Scene.h"
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

    for (auto* obj : objects)
        obj->Init(physics);

    physics.system.OptimizeBroadPhase();
}

void Scene::Update(float deltaTime)
{
    physics.Update(deltaTime);

    for (auto* obj : objects)
        obj->Update(physics, deltaTime);
}

void Scene::Draw()
{
    BeginMode3D(camera);
    for (auto* obj : objects)
        obj->Draw();
    EndMode3D();
}

void Scene::Shutdown()
{
    for (auto* obj : objects)
        obj->Shutdown(physics);

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
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;

    // Scroll para zoom
    float wheel = GetMouseWheelMove();
    cameraDistance -= wheel * 1.5f;
    cameraDistance = std::clamp(cameraDistance, cameraMinDistance, cameraMaxDistance);

    // Calcula posição da câmera baseado em yaw/pitch/distance
    float yawRad = cameraYaw * DEG2RAD;
    float pitchRad = cameraPitch * DEG2RAD;

    Vector3 offset;
    offset.x = cosf(pitchRad) * sinf(yawRad) * cameraDistance;
    offset.y = sinf(pitchRad) * cameraDistance;
    offset.z = cosf(pitchRad) * cosf(yawRad) * cameraDistance;

    // Target é um pouco acima do player
    cameraTarget = {playerPosition.x, playerPosition.y + 1.5f, playerPosition.z};

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
            // Coloca a câmera um pouco antes do ponto de colisão
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
