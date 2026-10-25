#pragma once
#include "Collider.h"
#include "Transform.h"

#include <glm/vec2.hpp>

#include <cstdint>

namespace Crumbs2D
{
class World;
struct BodyColumns;
template <typename T> struct Column;

class Body
{
private:
    World* m_World = nullptr;
    uint32_t m_Handle = 0;
    uint32_t m_Generation = 0;

private:
    friend class World;
    friend class Collider;
    Body(uint32_t handle, uint32_t generation, World* world)
        : m_World(world), m_Handle(handle), m_Generation(generation)
    {
    }

    template <typename T> const T& Value(Column<T> BodyColumns::* column) const;

public:
    class ColliderIterator
    {
    private:
        World* m_World = nullptr;
        uint32_t m_Handle = 0;
        uint32_t m_Generation = 0;

    public:
        ColliderIterator(World* world, uint32_t handle, uint32_t generation)
            : m_World(world), m_Handle(handle), m_Generation(generation)
        {
        }

        Collider operator*() const { return Collider(m_Handle, m_Generation, m_World); }
        ColliderIterator& operator++();
        bool operator!=(const ColliderIterator& other) const { return m_Handle != other.m_Handle; }
    };

    struct ColliderRange
    {
        ColliderIterator First;
        ColliderIterator begin() const { return First; }
        ColliderIterator end() const { return ColliderIterator(nullptr, UINT32_MAX, 0); }
    };

public:
    Body() = default;
    bool IsValid() const;

    void SetPosition(glm::vec2 position) const;
    glm::vec2 GetPosition() const;
    void SetRotation(float rotation) const;
    float GetRotation() const;
    Transform GetTransform() const;

    void SetVelocity(glm::vec2 velocity) const;
    glm::vec2 GetVelocity() const;
    void SetAngularVelocity(float angularVelocity) const;
    float GetAngularVelocity() const;

    void SetMass(float mass) const;
    float GetMass() const;
    void SetIsStatic(bool isStatic) const;
    bool IsStatic() const;

    void AddForce(glm::vec2 force) const;
    void AddTorque(float torque) const;
    void AddImpulse(glm::vec2 impulse) const;
    void AddAngularImpulse(float impulse) const;
    void AddForceAtPoint(glm::vec2 force, glm::vec2 point) const;
    void AddImpulseAtPoint(glm::vec2 impulse, glm::vec2 point) const;

    Collider AddCollider(const ColliderGeometry& geometry, const Transform& localTransform = {}) const;
    void DeleteCollider(const Collider& collider) const;
    uint32_t GetColliderCount() const;
    Collider GetCollider(uint32_t colliderIndex) const;
    ColliderRange GetColliders() const;

    bool operator==(const Body&) const = default;
};

} // namespace Crumbs2D