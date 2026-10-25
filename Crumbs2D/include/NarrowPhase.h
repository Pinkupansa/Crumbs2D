#pragma once
#include "Contact.h"
#include <unordered_map>
#include <vector>
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
    const std::vector<Contact>& GetContacts() const { return m_ContactBufferSwapchain[m_CurrentSwapchainIndex]; }

    template <typename F> void RemoveContactsIf(F&& shouldRemove)
    {
        std::vector<Contact>& contacts = m_ContactBufferSwapchain[m_CurrentSwapchainIndex];
        std::unordered_map<uint64_t, uint32_t>& cache = m_ContactCacheSwapchain[m_CurrentSwapchainIndex];

        for (uint32_t i = 0; i < contacts.size();)
        {
            if (!shouldRemove(contacts[i]))
            {
                i++;
                continue;
            }
            cache.erase(contacts[i].Colliders.MakePairKey());
            if (i != contacts.size() - 1)
            {
                contacts[i] = contacts.back();
                cache[contacts[i].Colliders.MakePairKey()] = i;
            }
            contacts.pop_back();
        }
    }
};
} // namespace Crumbs2D