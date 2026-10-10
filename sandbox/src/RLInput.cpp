#include "RLInput.h"

#include "raylib.h"
#include "imgui.h"

namespace
{
int ToRaylib(InputMouseButton button)
{
    switch (button)
    {
        case InputMouseButton::Left:
            return MOUSE_BUTTON_LEFT;
        case InputMouseButton::Right:
            return MOUSE_BUTTON_RIGHT;
        case InputMouseButton::Middle:
            return MOUSE_BUTTON_MIDDLE;
    }
    return MOUSE_BUTTON_LEFT;
}
} // namespace

glm::vec2 RLInput::GetMousePosition()
{
    const Vector2 p = ::GetMousePosition();
    return {p.x, p.y};
}

glm::vec2 RLInput::GetMouseDelta()
{
    const Vector2 d = ::GetMouseDelta();
    return {d.x, d.y};
}

float RLInput::GetMouseWheel() { return GetMouseWheelMove(); }

bool RLInput::IsMouseButtonDown(InputMouseButton button) { return ::IsMouseButtonDown(ToRaylib(button)); }
bool RLInput::IsMouseButtonPressed(InputMouseButton button) { return ::IsMouseButtonPressed(ToRaylib(button)); }

bool RLInput::IsMouseCapturedByUI() { return ImGui::GetIO().WantCaptureMouse; }