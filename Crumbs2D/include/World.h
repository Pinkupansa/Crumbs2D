#pragma once
#include "glm/vec2.hpp"
#include <cstdint>
#include "Body.h"
#include "Collider.h"
#include "Core.h"
#include <vector>
#include <functional>
namespace Crumbs2D
{
struct BodyStorage;
struct ColliderStorage;
class BroadPhase;
class NarrowPhase;
class ContactSolver;

struct PhysicsSettings
{ glm::vec2 Gravity = {0, 0}; };

struct SolverSettings
{
    float DeltaTime = 1.0f / 60.0f;
    int Iterations = 10;
    float Beta = 0.2f;
    float Slop = 0.005f;
};

// World.h, avant la classe World
struct ContactInfo
{
    Collider First;
    Collider Second;
    glm::vec2 Normal = {0.0f, 0.0f};
    uint32_t PointCount = 0;
    glm::vec2 Points[2] = {};
    float Separations[2] = {};
    uint32_t FeatureIDs[2] = {};
};

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
    Scope<BroadPhase> m_BroadPhase;
    Scope<NarrowPhase> m_NarrowPhase;
    Scope<ContactSolver> m_ContactSolver;

    SolverSettings m_SolverSettings;

private:
    friend class Body;
    friend class Collider;
    friend class BroadPhase;
    bool IsValid(const Body& body) const;
    bool IsValid(const Collider& collider) const;
    void SynchronizeColliders();

public:
    Crumbs2D::PhysicsSettings PhysicsSettings;

public:
    void Step();
    Body CreateBody();
    void DeleteBody(const Body& body);
    const SolverSettings& GetSolverSettings() const { return m_SolverSettings; }
    std::vector<std::pair<Collider, Collider>> GetCandidatePairs();
    void ForEachBroadPhaseNode(const std::function<void(const AABB&, uint16_t height, bool isLeaf)>& f) const;
    bool ValidateBroadPhase() const;
    std::vector<ContactInfo> GetContacts() const;
};
} // namespace Crumbs2D