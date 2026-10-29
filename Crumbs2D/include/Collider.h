#pragma once
#include <cstdint>
#include <glm/vec2.hpp>
#include "Geometry.h"

namespace Crumbs2D
{
class World;
class Body;
struct AABB;

struct ColliderColumns;
template <typename T> struct Column;

class Collider
{
private:
    World* m_World = nullptr;
    uint32_t m_Handle = 0;
    uint32_t m_Generation = 0;

private:
    friend class World;
    friend class Body;
    Collider(uint32_t handle, uint32_t generation, World* world)
        : m_World(world), m_Handle(handle), m_Generation(generation)
    {
    }

    template <typename T> const T& Value(Column<T> ColliderColumns::* column) const;

public:
    Collider() = default;
    bool IsValid() const;
    Body GetBody() const;

    Transform GetLocalTransform() const;
    void SetLocalTransform(const Transform& localTransform) const;
    glm::vec2 GetLocalPosition() const;
    void SetLocalPosition(glm::vec2 position) const;
    float GetLocalRotation() const;
    void SetLocalRotation(float rotation) const;

    // Center of the collider in its body's frame.
    glm::vec2 GetLocalCenter() const;

    Transform GetWorldTransform() const;
    glm::vec2 GetWorldPosition() const;
    float GetWorldRotation() const;
    AABB GetAABB() const;

    float GetFrictionCoefficient() const;
    void SetFrictionCoefficient(float frictionCoefficient) const;
    const ColliderGeometry& GetGeometry() const;
    void SetIsTrigger(bool trigger) const;
    bool operator==(const Collider&) const = default;
};

} // namespace Crumbs2D