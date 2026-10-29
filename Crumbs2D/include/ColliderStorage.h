#pragma once

#include "HandleTable.h"
#include "Geometry.h"
#include "Transform.h"
#include "WorldFlags.h"

#include <glm/vec2.hpp>

#include <cstdint>

namespace Crumbs2D
{
struct ColliderColumns
{
    // Structure
    Column<Id> Body{{}, {INVALID_HANDLE, 0}};
    Column<Id> PrevCollider{{}, {INVALID_HANDLE, 0}};
    Column<Id> NextCollider{{}, {INVALID_HANDLE, 0}};

    // Primary state
    Column<ColliderGeometry> Geometry{{}, ColliderGeometry::MakeBox({})};
    Column<Transform> LocalTransform{{}, Transform{}};
    Column<float> FrictionCoefficient{{}, 0.6f};
    Column<float> Bounciness{{}, 0.0f};
    Column<uint8_t> IsTrigger{{}, false};

    // Derived data
    Column<Transform> WorldTransform{{}, Transform{}};
    Column<Crumbs2D::AABB> AABB{{}, {{0.0f, 0.0f}, {1.0f, 1.0f}}};
    Column<uint32_t> Proxy{{}, UINT32_MAX};

    // Lifecycle
    Column<LifeState> State{{}, LifeState::Pending};
    Column<uint8_t> DirtyFlags{{}, 0};

    template <typename F> void ForEachColumn(F&& f)
    {
        f(Body);
        f(PrevCollider);
        f(NextCollider);
        f(Geometry);
        f(LocalTransform);
        f(FrictionCoefficient);
        f(WorldTransform);
        f(AABB);
        f(Proxy);
        f(State);
        f(DirtyFlags);
        f(Bounciness);
        f(IsTrigger);
    }
};

static_assert(ColumnSet<ColliderColumns>);
struct ColliderStorage : HandleTable<ColliderColumns>
{
};

} // namespace Crumbs2D