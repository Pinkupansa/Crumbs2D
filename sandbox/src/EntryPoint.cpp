#include "SandboxApp.h"
#include "SandboxScene.h"
#include "ExampleScene.h"
#include "Crumbs2D.h"

int main()
{

    SandboxApp app(Crumbs2D::CreateScope<ExampleScene>());
    app.Run();
}