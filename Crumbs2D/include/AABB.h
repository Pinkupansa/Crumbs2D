#pragma once
#include <glm/vec2.hpp>
namespace Crumbs2D
{
// Axis-aligned bounding box
struct AABB
{
    glm::vec2 min;
    glm::vec2 max;
    static inline bool TestOverlap(const AABB& a, const AABB& b)
    { return (a.min.x <= b.max.x) & (b.min.x <= a.max.x) & (a.min.y <= b.max.y) & (b.min.y <= a.max.y); }
};

} // namespace Crumbs2D