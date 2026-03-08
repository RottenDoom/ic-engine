#include "defines.h"
#include <GLFW/glfw3.h>

#include "core/input.h"
#include "core/application.h"
namespace ic
{

bool input::isKeyPressed(const KeyCode key)
{
        auto& window     = Application::get().getWindow();
        auto* glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());

        auto state       = glfwGetKey(glfwWindow, static_cast<int32_t>(key));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
}
bool input::isMouseButtonPressed(const MouseCode button)
{
        auto& window     = Application::get().getWindow();
        auto* glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());
        auto state       = glfwGetMouseButton(glfwWindow, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
}
std::pair<float, float> input::getMousePosition()
{
        auto& window     = Application::get().getWindow();
        auto* glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());
        double xpos, ypos;
        glfwGetCursorPos(glfwWindow, &xpos, &ypos);
        return {(float)xpos, (float)ypos};
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
}  // namespace ic
