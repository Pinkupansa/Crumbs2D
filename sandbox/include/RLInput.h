#pragma once

#include <glm/vec2.hpp>

enum class InputMouseButton { Left, Right, Middle };

class RLInput
{
public:
    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();
    static float GetMouseWheel();
    static bool IsMouseButtonDown(InputMouseButton button);
    static bool IsMouseButtonPressed(InputMouseButton button);
    // Vrai si la souris survole une fenêtre ImGui.
    static bool IsMouseCapturedByUI();
};