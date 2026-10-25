#pragma once

#include <cstdint>

namespace Crumbs2D
{
enum class LifeState : uint8_t { Pending, Active, PendingDestroy };

namespace BodyDirty
{
inline constexpr uint8_t Transform = 1 << 0;
inline constexpr uint8_t Mass = 1 << 1;
} // namespace BodyDirty

namespace ColliderDirty
{
inline constexpr uint8_t Placement = 1 << 0;
} // namespace ColliderDirty
} // namespace Crumbs2D