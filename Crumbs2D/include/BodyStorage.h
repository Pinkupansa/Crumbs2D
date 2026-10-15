#pragma once

#include "HandleTable.h"
#include "Collider.h"
#include "Geometry.h"
#include <glm/vec2.hpp>

#include <cstdint>

namespace Crumbs2D
{
struct BodyColumns
{

    Column<glm::vec2> Velocity{{}, glm::vec2(0.0f)};
    Column<glm::vec2> MassCenter{{}, glm::vec2(0.0f)};
    Column<float> AngularVelocity{{}, 0.0f};
    Column<float> Rotation{{}, 0.0f};
    Column<float> InverseMass{{}, 1.0f};
    Column<float> InverseInertia{{}, 1.0f};
    Column<glm::vec2> MassCenterLocal{{}, glm::vec2{0.0f, 0.0f}};
    Column<glm::vec2> Position{{}, glm::vec2{0.0f, 0.0f}};
    Column<glm::vec2> Force{{}, glm::vec2(0.0f)};
    Column<float> Torque{{}, 0.0f};

    Column<float> LinearDrag{{}, 1.0f};
    Column<float> AngularDrag{{}, 1.0f};

    Column<Id> FirstCollider{{}, {INVALID_HANDLE, 0}};
    Column<uint32_t> ColliderCount{{}, 0};

    Column<uint8_t> IsStatic{{}, 0};

    // BodyColumns
    template <typename F> void ForEachColumn(F&& f)
    {
        f(MassCenter);
        f(Rotation);
        f(Velocity);
        f(AngularVelocity);
        f(Force);
        f(Torque);
        f(InverseMass);
        f(InverseInertia);
        f(MassCenterLocal);
        f(LinearDrag);
        f(AngularDrag);
        f(FirstCollider);
        f(ColliderCount);
        f(IsStatic);
        f(Position);
    }
};

static_assert(ColumnSet<BodyColumns>);
struct BodyStorage : HandleTable<BodyColumns>
{
};

} // namespace Crumbs2D