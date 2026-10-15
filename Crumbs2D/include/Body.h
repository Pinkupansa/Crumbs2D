#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include "Collider.h"
namespace Crumbs2D
{
class World;
class ColliderGeometry;
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

    template <typename T> T& Value(Column<T> BodyColumns::* column) const;
    void RecalculateMassData() const;

public:
    Body() = default;
    bool IsValid() const;

    void SetVelocity(glm::vec2 velocity) const;
    glm::vec2 GetVelocity() const;

    void SetPosition(glm::vec2 position) const;
    glm::vec2 GetPosition() const;

    void SetAngularVelocity(float angularVelocity) const;
    float GetAngularVelocity() const;

    void SetRotation(float rotation) const;
    float GetRotation() const;

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

    Collider AddCollider(
        const ColliderGeometry& geometry, glm::vec2 localPosition = {0.0f, 0.0f}, float localRotation = 0.0f
    ) const;
    void DeleteCollider(const Collider& collider) const;
    Collider GetCollider(uint32_t colliderIndex) const;

    uint32_t GetColliderCount() const;

    class ColliderIterator
    {
    public:
        Collider operator*() const { return Collider(m_Handle, m_Generation, m_World); }
        ColliderIterator& operator++(); // défini dans Body.cpp : lit le stockage interne
        bool operator==(const ColliderIterator& other) const { return m_Handle == other.m_Handle; }

    private:
        friend class Body;
        ColliderIterator(World* world, uint32_t handle, uint32_t generation)
            : m_World(world), m_Handle(handle), m_Generation(generation)
        {
        }

        World* m_World;
        uint32_t m_Handle;
        uint32_t m_Generation;
    };

    // Plage parcourable avec une boucle for : for (Collider c : body.GetColliders())
    struct ColliderRange
    {
        ColliderIterator First;
        ColliderIterator begin() const { return First; }
        ColliderIterator end() const { return ColliderIterator(First.m_World, UINT32_MAX, 0); }
    };

    ColliderRange GetColliders() const;
};

} // namespace Crumbs2D