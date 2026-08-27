#ifndef LON_MODEL_ANIMATOR_H
#define LON_MODEL_ANIMATOR_H

struct LONModel;

namespace LONModelAnimator
{
    void SetAnimation(LONModel& model, int animIndex);
    void Update(LONModel& model, float deltaTime);
}

#endif // LON_MODEL_ANIMATOR_H
