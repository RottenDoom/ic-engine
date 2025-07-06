#pragma once

#include "keycodes.h"
#include "mousecodes.h"

namespace ic {
    class input
    {
    public:
        static bool isKeyPressed(KeyCode key);

        static bool isMouseButtonPressed(MouseCode button);
        static std::pair<float, float> getMousePosition();
        static float getMouseX();
        static float getMouseY();

    };
}