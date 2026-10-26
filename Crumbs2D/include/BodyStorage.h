#pragma once

#include "HandleTable.h"
#include "Collider.h"
#include "Geometry.h"
#include "WorldFlags.h"
#include <glm/vec2.hpp>

#include <cstdint>

namespace Crumbs2D
{
struct BodyColumns
{
    // Primary state
    Column<glm::vec2> Velocity{{}, glm::vec2(0.0f)};
    Column<float> AngularVelocity{{}, 0.0f};
    Column<glm::vec2> Position{{}, glm::vec2(0.0f)};
    Column<float> Rotation{{}, 0.0f};
    Column<glm::vec2> Force{{}, glm::vec2(0.0f)};
    Column<float> Torque{{}, 0.0f};
    Column<float> LinearDrag{{}, 0.0f};
    Column<float> AngularDrag{{}, 0.0f};
    Column<float> Mass{{}, 1.0f};
    Column<uint8_t> IsStatic{{}, 0};

    // Derived data
    Column<glm::vec2> MassCenter{{}, glm::vec2(0.0f)};
    Column<glm::vec2> MassCenterLocal{{}, glm::vec2(0.0f)};
    Column<float> InverseMass{{}, 1.0f};
    Column<float> InverseInertia{{}, 0.0f};

    // Structure
    Column<Id> FirstCollider{{}, {INVALID_HANDLE, 0}};
    Column<uint32_t> ColliderCount{{}, 0};

    // Lifecycle
    Column<LifeState> State{{}, LifeState::Pending};
    Column<uint8_t> DirtyFlags{{}, 0};

    template <typename F> void ForEachColumn(F&& f)
    {
        f(Velocity);
        f(AngularVelocity);
        f(Position);
        f(Rotation);
        f(Force);
        f(Torque);
        f(LinearDrag);
        f(AngularDrag);
        f(Mass);
        f(IsStatic);
        f(MassCenter);
        f(MassCenterLocal);
        f(InverseMass);
        f(InverseInertia);
        f(FirstCollider);
        f(ColliderCount);
        f(State);
        f(DirtyFlags);
    }
};

static_assert(ColumnSet<BodyColumns>);
struct BodyStorage : HandleTable<BodyColumns>
{
};

} // namespace Crumbs2D