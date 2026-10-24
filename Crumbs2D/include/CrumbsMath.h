#pragma once
#include "glm/vec2.hpp"
#include "math.h"
namespace Crumbs2D
{
inline float Cross(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }
inline glm::vec2 Rotate(glm::vec2 a, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return {c * a.x - s * a.y, c * a.y + s * a.x};
}
static constexpr float PI = 3.14159265358979323846f;
} // namespace Crumbs2D