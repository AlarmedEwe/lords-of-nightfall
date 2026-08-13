#ifndef PICKUP_OBJECT_H
#define PICKUP_OBJECT_H

#include "../GameObject.h"
#include "../Item.h"

struct PickupObject : public GameObject
{
    Item item;
    Model model = {};
    Texture2D iconTexture = {};
    bool isModelLoaded = false;
    bool isCollected = false;
    float rotationAngle = 0.0f;
    float bobbingTime = 0.0f;

    PickupObject(Item inItem, Vector3 inPos);

    void Init(Physics& physics) override;
    void Update(Physics& physics, float deltaTime) override;
    void Draw() override;
    void Shutdown(Physics& physics) override;
};

#endif // PICKUP_OBJECT_H
