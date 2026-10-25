#pragma once
#include "BodyStorage.h"
#include "ColliderStorage.h"
#include "Contact.h"
namespace Crumbs2D
{
struct ContactConstraintPoint
{
    float NormalLeverA;
    float NormalLeverB;
    float NormalMass;
    float DesiredNormalVelocity;

    float TangentialLeverA;
    float TangentialLeverB;
    float TangentialMass;
};

struct ContactConstraint
{
    Contact* Source;
    uint32_t IndexA;
    uint32_t IndexB;
    float InvMassA;
    float InvMassB;
    float InvInertiaA;
    float InvInertiaB;
    glm::vec2 Normal;
    glm::vec2 Tangent;
    float FrictionCoefficient;
    ContactConstraintPoint Points[2];
};
class ContactSolver
{
private:
    const uint16_t m_IterationNumber = 10;
    const float m_Beta = 0.2f;
    const float m_Slop = 0.01f;

    std::vector<ContactConstraint> m_Constraints;

public:
    ContactSolver(uint16_t iterationNumber, float beta, float slop)
        : m_IterationNumber(iterationNumber), m_Beta(beta), m_Slop(slop)
    {
    }
    void SolveContacts(
        BodyStorage& bodyStorage, ColliderStorage& colliderStorage, std::vector<Contact>& contacts, float deltaTime
    );
};
} // namespace Crumbs2D