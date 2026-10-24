#include "ContactSolver.h"
#include "CrumbsMath.h"
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <algorithm>
namespace Crumbs2D
{
// ContactSolver.cpp
void ContactSolver::SolveContacts(
    BodyStorage& bodyStorage, ColliderStorage& colliderStorage, std::vector<Contact>& contacts, float deltaTime
)
{
    m_Constraints.clear();

    for (Contact& contact : contacts)
    {
        ContactConstraint& constraint = m_Constraints.emplace_back();
        constraint.Source = &contact;
        constraint.BodyA = colliderStorage.Get(contact.Colliders.First(), &ColliderColumns::Body);
        constraint.BodyB = colliderStorage.Get(contact.Colliders.Second(), &ColliderColumns::Body);
        constraint.InvMassA = bodyStorage.Get(constraint.BodyA, &BodyColumns::InverseMass);
        constraint.InvMassB = bodyStorage.Get(constraint.BodyB, &BodyColumns::InverseMass);
        constraint.InvInertiaA = bodyStorage.Get(constraint.BodyA, &BodyColumns::InverseInertia);
        constraint.InvInertiaB = bodyStorage.Get(constraint.BodyB, &BodyColumns::InverseInertia);
        constraint.Normal = contact.Manifold.Normal;

        const glm::vec2 massCenterA = bodyStorage.Get(constraint.BodyA, &BodyColumns::MassCenter);
        const glm::vec2 massCenterB = bodyStorage.Get(constraint.BodyB, &BodyColumns::MassCenter);

        for (uint32_t i = 0; i < contact.Manifold.PointCount; i++)
        {
            ContactConstraintPoint& point = constraint.Points[i];
            const glm::vec2 leverArmA = contact.Manifold.Points[i].Position - massCenterA;
            const glm::vec2 leverArmB = contact.Manifold.Points[i].Position - massCenterB;
            point.NormalLeverA = Cross(leverArmA, constraint.Normal);
            point.NormalLeverB = Cross(leverArmB, constraint.Normal);

            point.NormalMass = 1.0f / (constraint.InvMassA + constraint.InvMassB +
                                       constraint.InvInertiaA * point.NormalLeverA * point.NormalLeverA +
                                       constraint.InvInertiaB * point.NormalLeverB * point.NormalLeverB);

            const float sep = contact.Manifold.Points[i].Separation;
            point.DesiredNormalVelocity =
                sep > 0.0f ? -sep / deltaTime : m_Beta / deltaTime * std::max(0.0f, -sep - m_Slop);

            contact.Manifold.Points[i].NormalImpulse = 0.0f;
            contact.Manifold.Points[i].TangentialImpulse = 0.0f;
        }
    }

    for (int it = 0; it < m_IterationNumber; it++)
    {
        for (ContactConstraint& constraint : m_Constraints)
        {
            glm::vec2& vA = bodyStorage.Get(constraint.BodyA, &BodyColumns::Velocity);
            float& wA = bodyStorage.Get(constraint.BodyA, &BodyColumns::AngularVelocity);
            glm::vec2& vB = bodyStorage.Get(constraint.BodyB, &BodyColumns::Velocity);
            float& wB = bodyStorage.Get(constraint.BodyB, &BodyColumns::AngularVelocity);

            ContactManifold& manifold = constraint.Source->Manifold;

            for (uint32_t i = 0; i < manifold.PointCount; i++)
            {
                const ContactConstraintPoint& point = constraint.Points[i];

                const float normalVelocity =
                    glm::dot(constraint.Normal, vB - vA) + wB * point.NormalLeverB - wA * point.NormalLeverA;
                float increment = point.NormalMass * (point.DesiredNormalVelocity - normalVelocity);
                const float newLambda = std::max(manifold.Points[i].NormalImpulse + increment, 0.0f);
                increment = newLambda - manifold.Points[i].NormalImpulse;
                manifold.Points[i].NormalImpulse = newLambda;

                vA = vA - constraint.InvMassA * increment * constraint.Normal;
                wA = wA - constraint.InvInertiaA * increment * point.NormalLeverA;
                vB = vB + constraint.InvMassB * increment * constraint.Normal;
                wB = wB + constraint.InvInertiaB * increment * point.NormalLeverB;
            }
        }
    }
}
} // namespace Crumbs2D