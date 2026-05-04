#include "defines.h"
#include "core/input.h"

bool ic_input_key_pressed(ic::KeyCode key)
{
        return ic::input::isKeyPressed(key);
}

bool ic_input_mouse_button_pressed(ic::MouseCode button)
{
        return ic::input::isMouseButtonPressed(button);
}