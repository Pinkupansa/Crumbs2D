#include "BroadPhase.h"
#include "ColliderStorage.h"
#include "Profiler.h"

#include <cassert>

namespace Crumbs2D
{
uint32_t BroadPhase::AddProxy(const AABB& aabb, uint32_t colliderHandle)
{
    const uint32_t proxy = m_Tree.CreateProxy(aabb, colliderHandle);
    MarkDirty(proxy);
    return proxy;
}

void BroadPhase::RemoveProxy(uint32_t proxy)
{
    m_Tree.RemoveProxy(proxy);
    MarkDirty(proxy);
}

bool BroadPhase::MoveProxy(uint32_t proxy, const AABB& aabb)
{
    const bool reinserted = m_Tree.MoveProxy(aabb, proxy);
    if (reinserted)
        MarkDirty(proxy);
    return reinserted;
}

void BroadPhase::UpdateProxies(const ColliderStorage& colliderStorage)
{
    PROFILE_FUNCTION();
    const ColliderColumns& colliders = colliderStorage.Columns;
    for (uint32_t i = 0; i < colliderStorage.Count(); i++)
    {
        assert(colliders.Proxy.Data[i] != UINT32_MAX && "Collider without a leaf: SynchronizeDirty must run first");
        MoveProxy(colliders.Proxy.Data[i], colliders.AABB.Data[i]);
    }
}

void BroadPhase::MarkDirty(uint32_t leaf)
{
    if (leaf >= m_IsDirty.size())
        m_IsDirty.resize(leaf + 1, false);
    if (m_IsDirty[leaf])
        return;
    m_IsDirty[leaf] = true;
    m_DirtyProxies.push_back(leaf);
}

const std::vector<ColliderPair>&
BroadPhase::FindPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage)
{
    PROFILE_FUNCTION();
    if (m_DirtyProxies.empty())
        return m_Pairs;
    PROFILE_VALUE("Dirty proxies", m_DirtyProxies.size());

    CleanPairs();
    FindNewPairs(bodyStorage, colliderStorage);

    for (const uint32_t leaf : m_DirtyProxies)
        m_IsDirty[leaf] = false;
    m_DirtyProxies.clear();
    return m_Pairs;
}

void BroadPhase::CleanPairs()
{
    for (uint32_t i = 0; i < m_Pairs.size();)
    {
        if (IsDirty(m_PairLeaves[i].A) || IsDirty(m_PairLeaves[i].B))
        {
            m_Pairs[i] = m_Pairs.back();
            m_PairLeaves[i] = m_PairLeaves.back();
            m_Pairs.pop_back();
            m_PairLeaves.pop_back();
        }
        else
            i++;
    }
}

void BroadPhase::FindNewPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage)
{
    for (const uint32_t leafA : m_DirtyProxies)
    {
        if (!m_Tree.IsValidProxy(leafA))
            continue;

        const Id idA = colliderStorage.IdOfHandle(m_Tree.GetUserId(leafA));
        const Id bodyA = colliderStorage.Get(idA, &ColliderColumns::Body);
        const bool staticA = bodyStorage.Get(bodyA, &BodyColumns::IsStatic) != 0;

        m_Tree.Query(m_Tree.GetFatBound(leafA), [&](uint32_t handleB) {
            const Id idB = colliderStorage.IdOfHandle(handleB);
            const auto colliderB = colliderStorage.Row(idB);
            const uint32_t leafB = colliderB[&ColliderColumns::Proxy];

            if (leafB == leafA || (leafB < leafA && IsDirty(leafB)))
                return;

            const Id bodyB = colliderB[&ColliderColumns::Body];
            if (bodyB.Handle == bodyA.Handle)
                return;
            if (staticA && bodyStorage.Get(bodyB, &BodyColumns::IsStatic) != 0)
                return;

            m_Pairs.emplace_back(idA, idB);
            m_PairLeaves.push_back({leafA, leafB});
        });
    }
}
} // namespace Crumbs2D