#pragma once
#include "Contact.h"
#include "ColliderStorage.h"
#include "BodyStorage.h"
namespace Crumbs2D
{
class NarrowPhase
{
private:
    uint16_t m_CurrentSwapchainIndex = 0;
    std::vector<Contact> m_ContactBufferSwapchain[2];
    std::unordered_map<uint64_t, uint32_t> m_ContactCacheSwapchain[2];

public:
    const std::vector<Contact>&
    FindContacts(const ColliderStorage& colliderStorage, const std::vector<ColliderPair>& candidates);
    std::vector<Contact>& GetContacts() { return m_ContactBufferSwapchain[m_CurrentSwapchainIndex]; }
};
} // namespace Crumbs2D