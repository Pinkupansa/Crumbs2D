#pragma once
#include "ColliderStorage.h"
#include "BodyStorage.h"
#include "Contact.h"
#include "DynamicTree.h"
namespace Crumbs2D
{
struct PairLeaves
{
    uint32_t A;
    uint32_t B;
};

class BroadPhase
{
public:
    uint32_t AddProxy(const AABB& aabb, uint32_t colliderHandle);
    void RemoveProxy(uint32_t proxy);
    bool MoveProxy(uint32_t proxy, const AABB& aabb);
    void UpdateProxies(const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>&
    FindPairsNaive(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>& FindPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>& GetPairs() const { return m_Pairs; }
    const DynamicTree& GetTree() const { return m_Tree; }

private:
    void FindNewPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage);
    bool IsDirty(uint32_t leaf) const { return leaf < m_IsDirty.size() && m_IsDirty[leaf]; }
    void CleanPairs();
    void MarkDirty(uint32_t leaf);

private:
    DynamicTree m_Tree;
    std::vector<ColliderPair> m_Pairs;
    std::vector<uint32_t> m_DirtyProxies;
    std::vector<bool> m_IsDirty;
    std::vector<PairLeaves> m_PairLeaves;
};
} // namespace Crumbs2D