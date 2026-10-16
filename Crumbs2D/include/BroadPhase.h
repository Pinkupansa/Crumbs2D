#pragma once
#include "ColliderStorage.h"
#include "BodyStorage.h"
#include "Contact.h"
namespace Crumbs2D
{
class BroadPhase
{
private:
    std::vector<ColliderPair> m_Pairs;

public:
    BroadPhase() {}

    const std::vector<ColliderPair>& FindPairs(const BodyStorage& bodyStorage, const ColliderStorage& colliderStorage);
    const std::vector<ColliderPair>& GetPairs() { return m_Pairs; }
};
} // namespace Crumbs2D