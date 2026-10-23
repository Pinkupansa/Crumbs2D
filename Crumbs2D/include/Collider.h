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
    { RefreshWorldPlacement(); }

    template <typename T> T& Value(Column<T> ColliderColumns::* column) const;

    void RefreshWorldPlacement() const;
    void OnLocalTransformChanged() const;

public:
    Collider() = default;
    bool IsValid() const;
    Body GetBody() const;

    Transform GetLocalTransform() const;
    void SetLocalTransform(const Transform& localTransform) const;
    Transform GetWorldTransform() const;
    // Collider.h
    glm::vec2 GetLocalPosition() const;
    void SetLocalPosition(glm::vec2 position) const;

    float GetLocalRotation() const;
    void SetLocalRotation(float rotation) const;

    glm::vec2 GetWorldPosition() const;
    float GetWorldRotation() const;

    glm::vec2 GetLocalCenter() const; // position of center in body frame

    float GetFrictionCoefficient() const;
    void SetFrictionCoefficient(float frictionCoefficient) const;
    const ColliderGeometry& GetGeometry() const;

    const AABB GetAABB() const;

    bool operator==(const Collider&) const = default;
};

} // namespace Crumbs2D