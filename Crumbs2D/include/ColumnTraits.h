#pragma once

#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "WorldFlags.h"

#include <cstdint>

namespace Crumbs2D
{
// Everything a write to a column invalidates. Columns not listed here are derived: only the world writes them.
template <auto Member> struct ColumnTraits
{
    static constexpr bool Writable = false;
    static constexpr uint8_t BodyFlags = 0;
    static constexpr uint8_t ColliderFlags = 0;
};

#define CRUMBS_WRITABLE_COLUMN(member, bodyFlags, colliderFlags)                                                       \
    template <> struct ColumnTraits<member>                                                                            \
    {                                                                                                                  \
        static constexpr bool Writable = true;                                                                         \
        static constexpr uint8_t BodyFlags = bodyFlags;                                                                \
        static constexpr uint8_t ColliderFlags = colliderFlags;                                                        \
    };

CRUMBS_WRITABLE_COLUMN(&BodyColumns::Position, BodyDirty::Transform, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::Rotation, BodyDirty::Transform, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::Mass, BodyDirty::Mass, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::IsStatic, BodyDirty::Mass, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::Velocity, 0, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::AngularVelocity, 0, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::Force, 0, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::Torque, 0, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::LinearDrag, 0, 0)
CRUMBS_WRITABLE_COLUMN(&BodyColumns::AngularDrag, 0, 0)

CRUMBS_WRITABLE_COLUMN(&ColliderColumns::LocalTransform, BodyDirty::Mass, ColliderDirty::Placement)
CRUMBS_WRITABLE_COLUMN(&ColliderColumns::FrictionCoefficient, 0, 0)
CRUMBS_WRITABLE_COLUMN(&ColliderColumns::Bounciness, 0, 0)
CRUMBS_WRITABLE_COLUMN(&ColliderColumns::IsTrigger, 0, 0);

#undef CRUMBS_WRITABLE_COLUMN

template <typename> struct MemberColumnType;
template <typename T, typename Owner> struct MemberColumnType<Column<T> Owner::*>
{
    using Type = T;
};
template <auto Member> using ColumnValue = typename MemberColumnType<decltype(Member)>::Type;
} // namespace Crumbs2D