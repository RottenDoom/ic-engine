#include "defines.h"

#include "core/input.h"
#include "core/application.h"

#include <GLFW/glfw3.h>

namespace ic {

	bool input::isKeyPressed(const KeyCode key)
	{
		auto window = static_cast<GLFWwindow*>(application::get().getWindow().getNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}
	bool input::isMouseButtonPressed(const MouseCode button)
	{
		auto window = static_cast<GLFWwindow*>(application::get().getWindow().getNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}
	std::pair<float, float> input::getMousePosition()
	{
		auto window = static_cast<GLFWwindow*>(application::get().getWindow().getNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return { (float)xpos, (float)ypos };
	}
	float input::getMouseX()
	{
		auto [x, y] = getMousePosition();
		return x;
	}
	float input::getMouseY()
	{
		auto [x, y] = getMousePosition();
		return y;
	}
} // namespace ic
