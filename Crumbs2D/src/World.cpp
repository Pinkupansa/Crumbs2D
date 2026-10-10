#include "World.h"

#include <cassert>

namespace Crumbs2D
{

bool World::IsValid(const Body& body) const
{
    return body.m_World == this && body.m_Handle < m_Generations.size() &&
           m_Generations[body.m_Handle] == body.m_Generation;
}

uint32_t World::GetIndex(const Body& body) const
{
    assert(IsValid(body) && "Body invalide : supprimé ou appartenant à un autre World");
    return m_HandleToIndex[body.m_Handle];
}

void World::SetVelocity(const Body& body, glm::vec2 velocity) { m_Velocities[GetIndex(body)] = velocity; }
glm::vec2 World::GetVelocity(const Body& body) const { return m_Velocities[GetIndex(body)]; }

void World::SetPosition(const Body& body, glm::vec2 position) { m_Positions[GetIndex(body)] = position; }
glm::vec2 World::GetPosition(const Body& body) const { return m_Positions[GetIndex(body)]; }

void World::SetAngularVelocity(const Body& body, float angularVelocity)
{ m_AngularVelocities[GetIndex(body)] = angularVelocity; }
float World::GetAngularVelocity(const Body& body) const { return m_AngularVelocities[GetIndex(body)]; }

void World::SetRotation(const Body& body, float rotation) { m_Rotations[GetIndex(body)] = rotation; }
float World::GetRotation(const Body& body) const { return m_Rotations[GetIndex(body)]; }

void World::SetMass(const Body& body, float mass)
{
    // Une masse nulle ou négative donne une masse inverse nulle : body immobile.
    m_InverseMasses[GetIndex(body)] = mass > 0.0f ? 1.0f / mass : 0.0f;
}

float World::GetMass(const Body& body) const
{
    const float inverseMass = m_InverseMasses[GetIndex(body)];
    return inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
}

void World::AddForce(const Body& body, glm::vec2 force) { m_Forces[GetIndex(body)] += force; }
void World::AddTorque(const Body& body, float torque) { m_Torques[GetIndex(body)] += torque; }

void World::Step(float dt)
{
    for (uint32_t i = 0; i < m_BodyNumber; i++)
    {
        m_Velocities[i] += dt * m_Forces[i] * m_InverseMasses[i];
        m_AngularVelocities[i] += dt * m_Torques[i] * m_InverseInertias[i];

        m_Positions[i] += dt * m_Velocities[i];
        m_Rotations[i] += dt * m_AngularVelocities[i];

        m_Forces[i] = glm::vec2(0.0f, 0.0f);
        m_Torques[i] = 0.0f;
    }
}

void World::ResetIndex(uint32_t index)
{
    m_AngularVelocities[index] = 0;
    m_Rotations[index] = 0;

    m_Velocities[index] = glm::vec2(0, 0);
    m_Positions[index] = glm::vec2(0, 0);

    m_Forces[index] = glm::vec2(0, 0);
    m_Torques[index] = 0;

    m_InverseMasses[index] = 1;
    m_InverseInertias[index] = 1;
}

Body World::AddBody()
{
    uint32_t handle;
    if (!m_FreeHandles.empty())
    {
        handle = m_FreeHandles.back();
        m_FreeHandles.pop_back();
    }
    else
    {
        handle = static_cast<uint32_t>(m_HandleToIndex.size());
        m_HandleToIndex.push_back(0);
        m_Generations.push_back(0);
    }

    const uint32_t index = m_BodyNumber++;
    ForEachArray([](auto& array) { array.emplace_back(); });
    ResetIndex(index);

    m_HandleToIndex[handle] = index;
    m_IndexToHandle[index] = handle;

    return Body(handle, m_Generations[handle], this);
}

void World::DeleteBody(const Body& body)
{
    const uint32_t index = GetIndex(body);
    const uint32_t handle = body.m_Handle;
    const uint32_t last = m_BodyNumber - 1;

    if (index != last)
    {
        ForEachArray([&](auto& array) { array[index] = array[last]; });
        m_HandleToIndex[m_IndexToHandle[index]] = index;
    }

    ForEachArray([](auto& array) { array.pop_back(); });
    m_BodyNumber--;

    m_Generations[handle]++;
    m_FreeHandles.push_back(handle);
}

} // namespace Crumbs2D