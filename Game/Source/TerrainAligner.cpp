#include "TerrainAligner.h"
#include "GameObjects/NatureObject.h"
#include "GameObjects/EnemyObject.h"
#include "GameObjects/PickupObject.h"
#include "GameObjects/PlayerObject.h"

void TerrainAligner::AlignObject(GameObject* obj, TerrainObject& terrain, Physics& physics)
{
    if (!obj) return;

    if (auto* prop = dynamic_cast<NatureObject*>(obj))
    {
        prop->position.y = terrain.GetHeightAt(prop->position.x, prop->position.z);

        if (prop->hasPhysicsBody && !prop->bodyId.IsInvalid())
        {
            float trunkHalfHeight = 2.5f * prop->scaleVal;
            JPH::Vec3 joltPos(prop->position.x, prop->position.y + trunkHalfHeight, prop->position.z);
            physics.system.GetBodyInterface().SetPosition(prop->bodyId, joltPos, JPH::EActivation::DontActivate);
        }
    }
    else if (auto* enemy = dynamic_cast<EnemyObject*>(obj))
    {
        float terrainY = terrain.GetHeightAt(enemy->position.x, enemy->position.z);
        float offset = (enemy->category == "Flying" ? 2.8f : 0.0f);
        enemy->position.y = terrainY + offset;
        enemy->spawnPos.y = terrain.GetHeightAt(enemy->spawnPos.x, enemy->spawnPos.z) + offset;
        enemy->patrolTarget.y = terrain.GetHeightAt(enemy->patrolTarget.x, enemy->patrolTarget.z) + offset;
    }
    else if (auto* pickup = dynamic_cast<PickupObject*>(obj))
    {
        pickup->position.y = terrain.GetHeightAt(pickup->position.x, pickup->position.z);
    }
    else if (auto* player = dynamic_cast<PlayerObject*>(obj))
    {
        player->position.y = terrain.GetHeightAt(player->position.x, player->position.z);
        if (player->character)
        {
            JPH::RVec3 joltPos(player->position.x, player->position.y + 0.9f, player->position.z);
            player->character->SetPosition(joltPos);
        }
    }
    else if (!dynamic_cast<TerrainObject*>(obj))
    {
        obj->position.y = terrain.GetHeightAt(obj->position.x, obj->position.z);
    }
}

void TerrainAligner::AlignAllInRadius(Scene& scene, TerrainObject& terrain, Physics& physics, float centerX, float centerZ, float radius)
{
    float checkRadius = radius + 3.0f;
    float checkRadiusSq = checkRadius * checkRadius;

    for (GameObject* obj : scene.objects)
    {
        if (!obj || dynamic_cast<TerrainObject*>(obj)) continue;

        float dx = obj->position.x - centerX;
        float dz = obj->position.z - centerZ;
        if (dx * dx + dz * dz <= checkRadiusSq)
        {
            AlignObject(obj, terrain, physics);
        }
    }
}

void TerrainAligner::AlignAll(Scene& scene, TerrainObject& terrain, Physics& physics)
{
    for (GameObject* obj : scene.objects)
    {
        if (!obj || dynamic_cast<TerrainObject*>(obj)) continue;
        AlignObject(obj, terrain, physics);
    }
}
