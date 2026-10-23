#pragma once
#include "ColliderStorage.h"
#include "BodyStorage.h"
#include "Contact.h"
#include "DynamicTree.h"
namespace Crumbs2D
{
class BroadPhase
{
public:
    uint32_t AddProxy(const AABB& aabb, uint32_t colliderHandle) { return m_Tree.CreateProxy(aabb, colliderHandle); }
    void RemoveProxy(uint32_t proxy) { m_Tree.RemoveProxy(proxy); }
    bool MoveProxy(uint32_t proxy, const AABB& aabb) { return m_Tree.MoveProxy(aabb, proxy); }
    void UpdateProxies(const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>&
    FindPairsNaive(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>& FindPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>& GetPairs() const { return m_Pairs; }
    const DynamicTree& GetTree() const { return m_Tree; }

private:
    DynamicTree m_Tree;
    std::vector<ColliderPair> m_Pairs;
};
} // namespace Crumbs2D