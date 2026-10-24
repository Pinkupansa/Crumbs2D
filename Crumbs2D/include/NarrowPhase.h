#pragma once
#include "Contact.h"
#include "ColliderStorage.h"
#include "BodyStorage.h"
namespace Crumbs2D
{
class NarrowPhase
{
private:
    std::vector<Contact> m_Contacts;

public:
    const std::vector<Contact>&
    FindContacts(const ColliderStorage& colliderStorage, const std::vector<ColliderPair>& candidates);
    std::vector<Contact>& GetContacts() { return m_Contacts; }
};
} // namespace Crumbs2D