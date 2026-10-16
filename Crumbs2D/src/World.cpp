#include "World.h"
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "Integrator.h"
#include <cassert>
#include "BroadPhase.h"
namespace Crumbs2D
{

World::World(const SolverSettings& solverSettings)
    : m_Bodies(CreateScope<BodyStorage>()), m_Colliders(CreateScope<ColliderStorage>()),
      m_BroadPhase(CreateScope<BroadPhase>()), m_SolverSettings(solverSettings)
{ assert(m_SolverSettings.DeltaTime > 0.0f && "DeltaTime must be positive"); }

World::~World() = default;
World::World(World&&) noexcept = default;
World& World::operator=(World&&) noexcept = default;

bool World::IsValid(const Body& body) const
{ return body.m_World == this && m_Bodies->IsValid({body.m_Handle, body.m_Generation}); }

bool World::IsValid(const Collider& collider) const
{ return collider.m_World == this && m_Colliders->IsValid({collider.m_Handle, collider.m_Generation}); }

void World::RecomputeAABBs()
{
    ColliderColumns& colliders = m_Colliders->Columns;
    const BodyStorage& bodies = *m_Bodies;

    for (uint32_t i = 0; i < m_Colliders->Count(); i++)
    {
        const Id bodyId = colliders.Body.Data[i];
        const glm::vec2 bodyPosition = bodies.Get(bodyId, &BodyColumns::Position);
        const float bodyRotation = bodies.Get(bodyId, &BodyColumns::Rotation);

        const glm::vec2 worldPosition = bodyPosition + Rotate(colliders.LocalPosition.Data[i], bodyRotation);
        const float worldRotation = bodyRotation + colliders.LocalRotation.Data[i];

        colliders.AABB.Data[i] =
            ColliderGeometry::RecomputeAABB(colliders.Geometry.Data[i], worldPosition, worldRotation);
    }
}

// World.cpp, avec #include <chrono>
void World::Step()
{
    assert(m_Bodies);
    using Clock = std::chrono::steady_clock;
    auto ms = [](Clock::time_point a, Clock::time_point b) {
        return std::chrono::duration<float, std::milli>(b - a).count();
    };

    const float dt = m_SolverSettings.DeltaTime;
    const Clock::time_point t0 = Clock::now();

    Integrator::IntegrateVelocities(*m_Bodies, PhysicsSettings, dt);
    const Clock::time_point t1 = Clock::now();

    Integrator::IntegratePositions(*m_Bodies, dt);
    const Clock::time_point t2 = Clock::now();

    RecomputeAABBs();
    const Clock::time_point t3 = Clock::now();

    const std::vector<ColliderPair>& pairs = m_BroadPhase->FindPairs(*m_Bodies, *m_Colliders);
    const Clock::time_point t4 = Clock::now();

    m_Profile = {ms(t0, t4), ms(t0, t1), ms(t1, t2), ms(t2, t3), ms(t3, t4), static_cast<uint32_t>(pairs.size())};
}
Body World::CreateBody()
{
    const auto id = m_Bodies->Add();
    return Body(id.Handle, id.Generation, this);
}

void World::DeleteBody(const Body& body)
{
    assert(body.m_World == this && "Body is not from this world");
    while (body.GetColliderCount() > 0)
        body.DeleteCollider(body.GetCollider(0));
    m_Bodies->Remove({body.m_Handle, body.m_Generation});
}

std::vector<std::pair<Collider, Collider>> World::GetCandidatePairs()
{
    std::vector<std::pair<Collider, Collider>> result;
    for (const ColliderPair& pair : m_BroadPhase->GetPairs())
        result.emplace_back(
            Collider(pair.First().Handle, pair.First().Generation, this),
            Collider(pair.Second().Handle, pair.Second().Generation, this)
        );
    return result;
}

} // namespace Crumbs2D