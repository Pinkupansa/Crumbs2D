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

void World::SetMass(const Body& body, float mass)
{
    // Une masse nulle ou négative donne une masse inverse nulle : body immobile.
    Field(body, &World::m_InverseMasses) = mass > 0.0f ? 1.0f / mass : 0.0f;
}

float World::GetMass(const Body& body) const
{
    const float inverseMass = Field(body, &World::m_InverseMasses);
    return inverseMass > 0.0f ? 1.0f / inverseMass : 0.0f;
}

void World::AddForce(const Body& body, glm::vec2 force) { Field(body, &World::m_Forces) += force; }
void World::AddTorque(const Body& body, float torque) { Field(body, &World::m_Torques) += torque; }

void World::Step(float dt)
{
    for (uint32_t i = 0; i < m_BodyCount; i++)
    {
        m_Velocities.data[i] +=
            dt * (m_Forces.data[i] * m_InverseMasses.data[i] + (float)(!m_IsStatic.data[i]) * m_Settings.Gravity);
        m_AngularVelocities.data[i] += dt * m_Torques.data[i] * m_InverseInertias.data[i];

        m_Positions.data[i] += dt * m_Velocities.data[i];
        m_Rotations.data[i] += dt * m_AngularVelocities.data[i];

        m_Forces.data[i] = glm::vec2(0.0f, 0.0f);
        m_Torques.data[i] = 0.0f;

        m_Velocities.data[i] /= (1 + m_LinearDrags.data[i] * dt);
        m_AngularVelocities.data[i] /= (1 + m_AngularDrags.data[i] * dt);
    }
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

    const uint32_t index = m_BodyCount++;
    ForEachColumn([](auto& column) { column.data.push_back(column.defaultValue); });

    m_HandleToIndex[handle] = index;
    m_IndexToHandle.data[index] = handle;

    return Body(handle, m_Generations[handle], this);
}

void World::DeleteBody(const Body& body)
{
    const uint32_t index = GetIndex(body);
    const uint32_t handle = body.m_Handle;
    const uint32_t last = m_BodyCount - 1;

    if (index != last)
    {
        ForEachColumn([&](auto& column) { column.data[index] = column.data[last]; });
        m_HandleToIndex[m_IndexToHandle.data[index]] = index;
    }

    ForEachColumn([](auto& column) { column.data.pop_back(); });
    m_BodyCount--;

    m_Generations[handle]++;
    m_FreeHandles.push_back(handle);
}

} // namespace Crumbs2D