#pragma once
#include "glm/vec2.hpp"
#include <cstdint>
#include <vector>
#include "Body.h"

namespace Crumbs2D
{
class World
{
private:
    uint32_t m_BodyNumber = 0;

    // Indexés par handle
    std::vector<uint32_t> m_FreeHandles;
    std::vector<uint32_t> m_HandleToIndex;
    std::vector<uint32_t> m_Generations;

    // Indexés par position dans les tableaux compacts
    std::vector<uint32_t> m_IndexToHandle;

    std::vector<glm::vec2> m_Velocities;
    std::vector<glm::vec2> m_Positions;
    std::vector<float> m_AngularVelocities;
    std::vector<float> m_Rotations;
    std::vector<float> m_InverseMasses;
    std::vector<float> m_InverseInertias;

    std::vector<glm::vec2> m_Forces;
    std::vector<float> m_Torques;

private:
    friend class Body;

    bool IsValid(const Body& body) const;
    uint32_t GetIndex(const Body& body) const;

    void SetVelocity(const Body& body, glm::vec2 velocity);
    glm::vec2 GetVelocity(const Body& body) const;

    void SetPosition(const Body& body, glm::vec2 position);
    glm::vec2 GetPosition(const Body& body) const;

    void SetAngularVelocity(const Body& body, float angularVelocity);
    float GetAngularVelocity(const Body& body) const;

    void SetRotation(const Body& body, float rotation);
    float GetRotation(const Body& body) const;

    void SetMass(const Body& body, float mass);
    float GetMass(const Body& body) const;

    void AddForce(const Body& body, glm::vec2 force);
    void AddTorque(const Body& body, float torque);
    void ResetIndex(uint32_t index);

    template <typename F> void ForEachArray(F&& f)
    {
        f(m_Positions);
        f(m_Rotations);
        f(m_Velocities);
        f(m_AngularVelocities);
        f(m_Forces);
        f(m_Torques);
        f(m_InverseMasses);
        f(m_InverseInertias);
        f(m_IndexToHandle);
    }

public:
    void Step(float dt);
    Body AddBody();
    void DeleteBody(const Body& body);
};
} // namespace Crumbs2D