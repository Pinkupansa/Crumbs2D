#pragma once
#include "CrumbsMath.h"
#include <glm/vec2.hpp>

namespace Crumbs2D
{
struct Transform
{
    glm::vec2 Position = {0.0f, 0.0f};
    float Rotation = 0.0f;

    static glm::vec2 ApplyToPoint(const Transform& t, glm::vec2 localPoint)
    { return t.Position + Rotate(localPoint, t.Rotation); }

    static glm::vec2 ApplyInverseToPoint(const Transform& t, glm::vec2 point)
    { return Rotate(point - t.Position, -t.Rotation); }

    static glm::vec2 ApplyToVector(const Transform& t, glm::vec2 v) { return Rotate(v, t.Rotation); }
    static glm::vec2 ApplyInverseToVector(const Transform& t, glm::vec2 v) { return Rotate(v, -t.Rotation); }

    static Transform Compose(const Transform& parent, const Transform& child)
    { return {ApplyToPoint(parent, child.Position), parent.Rotation + child.Rotation}; }

    static Transform Relative(const Transform& a, const Transform& b)
    { return {ApplyInverseToPoint(a, b.Position), b.Rotation - a.Rotation}; }
    glm::vec2 Right() const { return {std::cos(Rotation), std::sin(Rotation)}; }

    glm::vec2 Up() const { return {-std::sin(Rotation), std::cos(Rotation)}; }
};
} // namespace Crumbs2D