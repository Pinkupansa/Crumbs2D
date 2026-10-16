#include "BroadPhase.h"
#include "ColliderStorage.h"
namespace Crumbs2D
{
// BroadPhase.cpp
const std::vector<ColliderPair>&
BroadPhase::FindPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage)
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
} // namespace Crumbs2D