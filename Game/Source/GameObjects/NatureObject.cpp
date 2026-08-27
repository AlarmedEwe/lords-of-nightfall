#include "NatureObject.h"
#include "TerrainObject.h"
#include "raymath.h"
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <unordered_map>

static std::unordered_map<std::string, LONModel*> s_NatureModelCache;

NatureObject::NatureObject(const std::string& modelFile, Vector3 pos, float yaw, float scaleVal, bool isTree)
    : modelFile(modelFile), isTree(isTree), yaw(yaw), scaleVal(scaleVal)
{
    position = pos;
}

void NatureObject::Init(Physics& physics)
{
    auto it = s_NatureModelCache.find(modelFile);
    if (it != s_NatureModelCache.end())
    {
        natureModel = it->second;
    }
    else
    {
        LONModel* model = new LONModel();
        model->Load(modelFile.c_str(), "");
        s_NatureModelCache[modelFile] = model;
        natureModel = model;
    }

    if (TerrainObject::s_Instance)
    {
        position.y = TerrainObject::s_Instance->GetHeightAt(position.x, position.z);
    }

    if (isTree)
    {
        // Colisor físico do tronco para árvores
        float trunkRadius = 0.45f * scaleVal;
        float trunkHalfHeight = 2.5f * scaleVal;

        JPH::CylinderShapeSettings shapeSettings(trunkHalfHeight, trunkRadius);
        JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();

        if (shapeResult.IsValid())
        {
            JPH::Vec3 joltPos(position.x, position.y + trunkHalfHeight, position.z);
            JPH::BodyCreationSettings bodySettings(
                shapeResult.Get(),
                joltPos,
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Static,
                Layers::NON_MOVING
            );

            JPH::BodyInterface& bodyInterface = physics.system.GetBodyInterface();
            JPH::Body* body = bodyInterface.CreateBody(bodySettings);
            if (body)
            {
                bodyId = body->GetID();
                bodyInterface.AddBody(bodyId, JPH::EActivation::DontActivate);
                hasPhysicsBody = true;
            }
        }
    }
}

void NatureObject::Update(Physics& physics, float deltaTime)
{
    (void)physics;
    (void)deltaTime;
}

void NatureObject::Draw()
{
    if (natureModel && natureModel->isLoaded)
    {
        natureModel->Draw(position, yaw, scaleVal);
    }
}

void NatureObject::Shutdown(Physics& physics)
{
    if (hasPhysicsBody)
    {
        JPH::BodyInterface& bodyInterface = physics.system.GetBodyInterface();
        bodyInterface.RemoveBody(bodyId);
        bodyInterface.DestroyBody(bodyId);
        hasPhysicsBody = false;
    }
}

void NatureObject::ClearCache()
{
    for (auto& pair : s_NatureModelCache)
    {
        if (pair.second)
        {
            pair.second->Unload();
            delete pair.second;
        }
    }
    s_NatureModelCache.clear();
}
