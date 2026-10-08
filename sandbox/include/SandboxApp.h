#pragma once

#include "Crumbs2D.h"
#include "SandboxScene.h"

class SandboxApp
{
public:
    explicit SandboxApp(Crumbs2D::Scope<SandboxScene> scene);

    void Run();

private:
    Crumbs2D::Scope<SandboxScene> m_Scene;
};