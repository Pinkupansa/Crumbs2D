#pragma once
#include "glm/vec2.hpp"
#include <cstdint>
#include "Body.h"
#include "Collider.h"
#include "Core.h"
namespace Crumbs2D
{
struct BodyStorage;
struct ColliderStorage;

struct PhysicsSettings
{ glm::vec2 Gravity = {0, 0}; };

struct SolverSettings
{ float DeltaTime = 1.0f / 60.0f; };

class World
{
public:
    explicit World(const SolverSettings& solverSettings = SolverSettings{});
    ~World();

    World(World&&) noexcept;
    World& operator=(World&&) noexcept;
    World(const World&) = delete;
    World& operator=(const World&) = delete;

private:
    Scope<BodyStorage> m_Bodies;
    Scope<ColliderStorage> m_Colliders;

    SolverSettings m_SolverSettings;

private:
    friend class Body;
    friend class Collider;
    bool IsValid(const Body& body) const;
    bool IsValid(const Collider& collider) const;
    void RecomputeAABBs();

public:
    Crumbs2D::PhysicsSettings PhysicsSettings;

public:
    void Step();
    Body CreateBody();
    void DeleteBody(const Body& body);
    const SolverSettings& GetSolverSettings() const { return m_SolverSettings; }
};
} // namespace Crumbs2D