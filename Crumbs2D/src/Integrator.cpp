#include "Integrator.h"
#include "BodyStorage.h"
#include "World.h"
#include "Profiler.h"
namespace Crumbs2D::Integrator
{
void IntegrateVelocities(BodyStorage& bodies, const PhysicsSettings& settings, float dt)
{
    PROFILE_FUNCTION();
    BodyColumns& c = bodies.Columns;

    for (uint32_t i = 0; i < bodies.Count(); i++)
    {
        c.Velocity.Data[i] +=
            dt * (c.Force.Data[i] * c.InverseMass.Data[i] + (float)(!c.IsStatic.Data[i]) * settings.Gravity);
        c.AngularVelocity.Data[i] += dt * c.Torque.Data[i] * c.InverseInertia.Data[i];

        c.Velocity.Data[i] /= (1 + c.LinearDrag.Data[i] * dt);
        c.AngularVelocity.Data[i] /= (1 + c.AngularDrag.Data[i] * dt);

        c.Force.Data[i] = glm::vec2(0.0f, 0.0f);
        c.Torque.Data[i] = 0.0f;
    }
}

void IntegratePositions(BodyStorage& bodies, float dt)
{
    PROFILE_FUNCTION();

    BodyColumns& c = bodies.Columns;
    for (uint32_t i = 0; i < bodies.Count(); i++)
    {
        c.MassCenter.Data[i] += dt * c.Velocity.Data[i];
        c.Rotation.Data[i] += dt * c.AngularVelocity.Data[i];
        c.Position.Data[i] = c.MassCenter.Data[i] - Rotate(c.MassCenterLocal.Data[i], c.Rotation.Data[i]);
    }
}
} // namespace Crumbs2D::Integrator