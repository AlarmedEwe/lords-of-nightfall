#include "LONModel.h"
#include "Model/LONModelIO.h"
#include "Model/LONModelAnimator.h"

void LONModel::Load(const char* lonmodelPath, const char* texturePath)
{
    LONModelIO::Load(*this, lonmodelPath, texturePath);
}

void LONModel::SetAnimation(int animIndex)
{
    LONModelAnimator::SetAnimation(*this, animIndex);
}

void LONModel::Update(float deltaTime)
{
    LONModelAnimator::Update(*this, deltaTime);
}

void LONModel::Draw(Vector3 position, float yaw, float scale, Color tint)
{
    if (!isLoaded || rayModel.meshCount == 0) return;

    Vector3 axis = { 0.0f, 1.0f, 0.0f };
    DrawModelEx(rayModel, position, axis, yaw, (Vector3){ scale, scale, scale }, tint);
}

void LONModel::Unload()
{
    LONModelIO::Unload(*this);
}
