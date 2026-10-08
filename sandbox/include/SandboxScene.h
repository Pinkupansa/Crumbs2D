#pragma once

class SandboxScene
{
public:
    virtual void OnInit() {}
    virtual void OnUpdate(float dt) {};
    virtual void OnDraw() {};
    virtual void OnImGuiRender() {};
};