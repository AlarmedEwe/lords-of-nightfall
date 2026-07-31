#include "Physics.h"
#include <cstdarg>
#include <iostream>

using namespace JPH;
using namespace JPH::literals;

static void TraceImpl(const char* inFMT, ...)
{
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << "[Jolt] " << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
{
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
    return true;
}
#endif

void Physics::Init()
{
    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

    Factory::sInstance = new Factory();
    RegisterTypes();

    tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
    jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, (int)std::thread::hardware_concurrency() - 1);

    const uint maxBodies = 4096;
    const uint numBodyMutexes = 0;
    const uint maxBodyPairs = 4096;
    const uint maxContactConstraints = 4096;

    system.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
                broadPhaseLayerInterface, objectVsBroadPhaseFilter, objectLayerPairFilter);

    system.SetGravity(Vec3(0.0f, -20.0f, 0.0f));
}

void Physics::Update(float deltaTime)
{
    // Jolt recomenda steps de 1/60. Se deltaTime for maior, fazemos múltiplos steps.
    const float fixedStep = 1.0f / 60.0f;
    int steps = (int)std::ceil(deltaTime / fixedStep);
    if (steps < 1) steps = 1;
    if (steps > 4) steps = 4;

    system.Update(deltaTime, steps, tempAllocator, jobSystem);
}

void Physics::Shutdown()
{
    UnregisterTypes();

    delete Factory::sInstance;
    Factory::sInstance = nullptr;

    delete jobSystem;
    jobSystem = nullptr;

    delete tempAllocator;
    tempAllocator = nullptr;
}

bool Physics::CastRay(Vec3 origin, Vec3 direction, float maxDistance, RVec3& outHitPoint)
{
    RRayCast ray(RVec3(origin.GetX(), origin.GetY(), origin.GetZ()), direction * maxDistance);
    RayCastResult result;

    bool hit = system.GetNarrowPhaseQuery().CastRay(ray, result);
    if (hit)
    {
        outHitPoint = ray.GetPointOnRay(result.mFraction);
    }
    return hit;
}
