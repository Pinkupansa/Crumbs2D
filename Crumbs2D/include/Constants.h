#pragma once

namespace Crumbs2D
{
inline constexpr float EPSILON = 1e-6f;

// BROAD PHASE
inline constexpr float AABB_FATTENING = 0.5f;
inline constexpr float DESCENT_FACTOR = 2.0f;

// NARROW PHASE
inline constexpr float SPECULATIVE_DISTANCE = 0.1f;
} // namespace Crumbs2D