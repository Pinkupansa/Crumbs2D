
#pragma once

#include "HandleTable.h"

#include <glm/vec2.hpp>

#include <cstdint>
#include "Geometry.h"
#include "Transform.h"
namespace Crumbs2D
{

struct ColliderColumns
{

    Column<Id> Body{{}, {INVALID_HANDLE, 0}};
    Column<Id> PrevCollider{{}, {INVALID_HANDLE, 0}};
    Column<Id> NextCollider{{}, {INVALID_HANDLE, 0}};

    Column<Transform> LocalTransform{{}, Transform{}};

    Column<ColliderGeometry> Geometry{{}, ColliderGeometry::MakeBox({})};
    Column<AABB> AABB{{}, {{0.0f, 0.0f}, {1.0f, 1.0f}}};
    Column<Transform> WorldTransform{{}, Transform{}};

    Column<float> FrictionCoefficient{{}, 0.6f};
    Column<uint32_t> Proxy{{}, UINT32_MAX};

    template <typename F> void ForEachColumn(F&& f)
    {
        f(Body);
        f(PrevCollider);
        f(NextCollider);
        f(Geometry);
        f(AABB);
        f(FrictionCoefficient);
        f(Proxy);
        f(LocalTransform);
        f(WorldTransform);
    }
};

static_assert(ColumnSet<ColliderColumns>);
struct ColliderStorage : HandleTable<ColliderColumns>
{
};

} // namespace Crumbs2D