#include "BroadPhase.h"
#include "ColliderStorage.h"
#include "Constants.h"
#include <iostream>
namespace Crumbs2D
{

// BroadPhase.cpp
const std::vector<ColliderPair>&
BroadPhase::FindPairsNaive(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage)
{
    m_Pairs.clear();

    const ColliderColumns& colliders = colliderStorage.Columns;
    const uint32_t count = colliderStorage.Count();

    for (uint32_t i = 0; i < count; i++)
    {
        const Id bodyA = colliders.Body.Data[i];
        const bool staticA = bodyStorage.Get(bodyA, &BodyColumns::IsStatic) != 0;

        for (uint32_t j = i + 1; j < count; j++)
        {
            const Id bodyB = colliders.Body.Data[j];
            if (bodyA.Handle == bodyB.Handle)
                continue;
            if (staticA && bodyStorage.Get(bodyB, &BodyColumns::IsStatic) != 0)
                continue;
            if (AABB::TestOverlap(colliders.AABB.Data[i], colliders.AABB.Data[j]))
                m_Pairs.push_back(ColliderPair(colliderStorage.IdAt(i), colliderStorage.IdAt(j)));
        }
    }
    return m_Pairs;
}

const std::vector<ColliderPair>&
BroadPhase::FindPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage)
{
    m_Pairs.clear();
    const ColliderColumns& colliders = colliderStorage.Columns;
    const uint32_t count = colliderStorage.Count();
    for (uint32_t i = 0; i < count; i++)
    {
        const Id idA = colliderStorage.IdAt(i);
        const Id bodyA = colliders.Body.Data[i];
        const bool staticA = bodyStorage.Get(bodyA, &BodyColumns::IsStatic) != 0;
        const AABB& aabbA = colliders.AABB.Data[i];

        m_Tree.Query(aabbA, [&](uint32_t handleB) {
            if (handleB <= idA.Handle)
                return;

            const Id idB = colliderStorage.IdOfHandle(handleB);
            const Id bodyB = colliderStorage.Get(idB, &ColliderColumns::Body);

            if (bodyB.Handle == bodyA.Handle)
                return;
            if (staticA && bodyStorage.Get(bodyB, &BodyColumns::IsStatic) != 0)
                return;

            if (AABB::TestOverlap(
                    AABB::Fatten(aabbA, SPECULATIVE_DISTANCE),
                    AABB::Fatten(colliderStorage.Get(idB, &ColliderColumns::AABB), SPECULATIVE_DISTANCE)
                ))
            {
                m_Pairs.push_back(ColliderPair(idA, idB));
            }
        });
    }

    return m_Pairs;
}
void BroadPhase::UpdateProxies(const ColliderStorage& colliderStorage)
{
    const ColliderColumns& colliders = colliderStorage.Columns;
    for (uint32_t i = 0; i < colliderStorage.Count(); i++)
        m_Tree.MoveProxy(colliders.AABB.Data[i], colliders.Proxy.Data[i]);
}
} // namespace Crumbs2D