#pragma once
#include "glm/vec2.hpp"
namespace Crumbs2D
{
float Cross(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }
} // namespace Crumbs2D