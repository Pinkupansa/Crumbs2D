#include "SandboxApp.h"
#include "SandboxScene.h"
#include "ExampleScene.h"
#include "BroadPhaseScene.h"
#include "StressScene.h"
#include "Crumbs2D.h"

int main()
{

    SandboxApp app(Crumbs2D::CreateScope<StressScene>());
    app.Run();
}