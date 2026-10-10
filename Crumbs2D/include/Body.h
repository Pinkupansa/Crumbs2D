#pragma once

#include <cstdint>
#include <glm/vec2.hpp>

namespace Crumbs2D
{
class World;

// Handle léger vers un body stocké dans un World.
// Copiable librement : toutes les copies désignent le même body.
// La génération permet de détecter l'utilisation d'un body supprimé.
class Body
{
private:
    World* m_World = nullptr;
    uint32_t m_Handle = 0;
    uint32_t m_Generation = 0;

private:
    friend class World;
    Body(uint32_t handle, uint32_t generation, World* world)
        : m_World(world), m_Handle(handle), m_Generation(generation)
    {
    }

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

    void AddForce(glm::vec2 force) const;
    void AddTorque(float torque) const;
};

} // namespace Crumbs2D