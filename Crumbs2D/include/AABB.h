#pragma once
#include <glm/vec2.hpp>
#include <glm/common.hpp>
namespace Crumbs2D
{
// Axis-aligned bounding box
struct AABB
{
    glm::vec2 min;
    glm::vec2 max;
    static inline bool TestOverlap(const AABB& a, const AABB& b)
    { return (a.min.x <= b.max.x) & (b.min.x <= a.max.x) & (a.min.y <= b.max.y) & (b.min.y <= a.max.y); }

    static AABB Combine(const AABB& a, const AABB& b) { return {glm::min(a.min, b.min), glm::max(a.max, b.max)}; }

    static float Cost(const AABB& a)
    {
        const glm::vec2 size = a.max - a.min;
        return 2.0f * (size.x + size.y);
    }

    static bool Contains(const AABB& outer, const AABB& inner)
    {
        return (outer.min.x <= inner.min.x) & (outer.min.y <= inner.min.y) & (inner.max.x <= outer.max.x) &
               (inner.max.y <= outer.max.y);
    }

    static AABB Fatten(const AABB& a, float margin) { return {a.min - glm::vec2(margin), a.max + glm::vec2(margin)}; }
};

} // namespace Crumbs2D