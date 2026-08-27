#ifndef NATURE_OBJECT_H
#define NATURE_OBJECT_H

#include "../GameObject.h"
#include "../LONModel.h"
#include <string>

struct NatureObject : public GameObject
{
    LONModel* natureModel = nullptr;
    std::string modelFile;
    bool isTree = false;
    float yaw = 0.0f;
    float scaleVal = 1.0f;

    JPH::BodyID bodyId;
    bool hasPhysicsBody = false;

    NatureObject(const std::string& modelFile, Vector3 pos, float yaw = 0.0f, float scaleVal = 1.0f, bool isTree = false);
    virtual ~NatureObject() override = default;

    virtual void Init(Physics& physics) override;
    virtual void Update(Physics& physics, float deltaTime) override;
    virtual void Draw() override;
    virtual void Shutdown(Physics& physics) override;

    static void ClearCache();
};

#endif // NATURE_OBJECT_H
