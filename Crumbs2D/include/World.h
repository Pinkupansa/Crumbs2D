#pragma once
#include "glm/vec2.hpp"
#include <cstdint>
#include <vector>
#include "Body.h"

namespace Crumbs2D
{
struct WorldSettings
{ glm::vec2 Gravity = {0, -9.81f}; };

// Un tableau compact et la valeur donnée à chaque nouvelle case.
template <typename T> struct Column
{
    std::vector<T> data;
    T defaultValue;
};

class World
{
private:
    WorldSettings m_Settings;
    uint32_t m_BodyCount = 0;

    // Indexés par handle
    std::vector<uint32_t> m_FreeHandles;
    std::vector<uint32_t> m_HandleToIndex;
    std::vector<uint32_t> m_Generations;

    // Indexés par position dans les tableaux compacts
    Column<uint32_t> m_IndexToHandle{{}, 0};

    Column<glm::vec2> m_Velocities{{}, glm::vec2(0.0f)};
    Column<glm::vec2> m_Positions{{}, glm::vec2(0.0f)};
    Column<float> m_AngularVelocities{{}, 0.0f};
    Column<float> m_Rotations{{}, 0.0f};
    Column<float> m_InverseMasses{{}, 1.0f};
    Column<float> m_InverseInertias{{}, 1.0f};

    Column<float> m_LinearDrags{{}, 1.0f};
    Column<float> m_AngularDrags{{}, 1.0f};

    Column<glm::vec2> m_Forces{{}, glm::vec2(0.0f)};
    Column<float> m_Torques{{}, 0.0f};

    // uint8_t et non bool : std::vector<bool> ne renvoie pas de vraie référence.
    Column<uint8_t> m_IsStatic{{}, 0};

private:
    friend class Body;

    bool IsValid(const Body& body) const;
    uint32_t GetIndex(const Body& body) const;

    template <typename T> T& Field(const Body& body, Column<T> World::* column)
    { return (this->*column).data[GetIndex(body)]; }

    template <typename T> const T& Field(const Body& body, Column<T> World::* column) const
    { return (this->*column).data[GetIndex(body)]; }

    void SetMass(const Body& body, float mass);
    float GetMass(const Body& body) const;

    void AddForce(const Body& body, glm::vec2 force);
    void AddTorque(const Body& body, float torque);

    template <typename F> void ForEachColumn(F&& f)
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
        f(m_IsStatic);
        f(m_LinearDrags);
        f(m_AngularDrags);
    }

public:
    void Step(float dt);
    Body AddBody();
    void DeleteBody(const Body& body);
};
} // namespace Crumbs2D