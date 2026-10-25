#pragma once

#include "ColumnTraits.h"
#include "World.h"

#include <type_traits>

namespace Crumbs2D
{
template <auto Member, typename T> void World::SetBodyValue(Id body, const T& value)
{
    using Traits = ColumnTraits<Member>;
    static_assert(Traits::Writable, "Derived column: only the world writes it, during synchronization");
    static_assert(std::is_convertible_v<T, ColumnValue<Member>>);

    m_Bodies->Get(body, Member) = static_cast<ColumnValue<Member>>(value);
    if constexpr (Traits::BodyFlags != 0)
        MarkBodyDirty(body, Traits::BodyFlags);
}

template <auto Member, typename T> void World::SetColliderValue(Id collider, const T& value)
{
    using Traits = ColumnTraits<Member>;
    static_assert(Traits::Writable, "Derived column: only the world writes it, during synchronization");
    static_assert(std::is_convertible_v<T, ColumnValue<Member>>);

    m_Colliders->Get(collider, Member) = static_cast<ColumnValue<Member>>(value);
    if constexpr (Traits::ColliderFlags != 0)
        MarkColliderDirty(collider, Traits::ColliderFlags);
    if constexpr (Traits::BodyFlags != 0)
        MarkBodyDirty(m_Colliders->Get(collider, &ColliderColumns::Body), Traits::BodyFlags);
}
} // namespace Crumbs2D