
#pragma once

#include "HandleTable.h"

#include <glm/vec2.hpp>

#include <cstdint>
#include "Geometry.h"
namespace Crumbs2D
{

struct ColliderColumns
{

    Column<Id> Body{{}, {INVALID_HANDLE, 0}};
    Column<Id> PrevCollider{{}, {INVALID_HANDLE, 0}};
    Column<Id> NextCollider{{}, {INVALID_HANDLE, 0}};

    Column<glm::vec2> LocalPosition{{}, glm::vec2(0.0f, 0.0f)};
    Column<float> LocalRotation{{}, 0};
    Column<ColliderGeometry> Geometry{{}, ColliderGeometry::MakeBox({})};
    Column<AABB> AABB{{}, {{0.0f, 0.0f}, {1.0f, 1.0f}}};
    Column<float> FrictionCoefficient{{}, 0.6f};
    // ColliderColumns
    template <typename F> void ForEachColumn(F&& f)
    {
        f(Body);
        f(PrevCollider);
        f(NextCollider);
        f(LocalPosition);
        f(LocalRotation);
        f(Geometry);
        f(AABB);
        f(FrictionCoefficient);
    }
};

static_assert(ColumnSet<ColliderColumns>);
struct ColliderStorage : HandleTable<ColliderColumns>
{
};

} // namespace Crumbs2D