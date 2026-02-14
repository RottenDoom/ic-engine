#ifndef INPUT_H
#define INPUT_H

#include "keycodes.h"
#include "mousecodes.h"

namespace ic
{
class input
{
public:
        static bool isKeyPressed(KeyCode key);

        static bool isMouseButtonPressed(MouseCode button);
        static std::pair<float, float> getMousePosition();
        static float getMouseX();
        static float getMouseY();
};
}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif

        /**
         * @function ic_input_key_pressed
         * @category input
         * @brief Checks for keyboard inputs. For keycodes check <core/keycodes.h>
         * @param ic::KeyCode enum value for keycodes
         * @example > Check for input in the app update function
         * 	#include <ic_engine.h>
         * 	void update(float deltaTime) {
         * 		if (ic_input_key_pressed(ic::KeyCode::W))
         * 			IC_TRACE("Move Forward");
         *      }
         */
        IC_API bool ic_input_key_pressed(ic::KeyCode key);

        /**
         * @function ic_input_mouse_button_pressed
         * @category input
         * @brief Checks for mouse input. For mousecodes check <core/mousecodes.h>
         * @param ic::MouseCode enum value for the mousecodes
         */
        IC_API bool ic_input_mouse_button_pressed(ic::MouseCode button);

        // IC_API float ic_get_mouse_pos(void);

#ifdef __cplusplus
}
#endif

#endif