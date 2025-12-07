#include "opengl_renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace ic
{

bool OpenGLRenderer::init(Window& window)
{
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                IC_CORE_ERROR("Failed to initialize GLAD");
                return false;
        }

        glEnable(GL_DEPTH_TEST);
        return true;
}

}  // namespace ic
