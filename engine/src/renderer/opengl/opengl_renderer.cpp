#include "opengl_renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl_debug.h"

namespace ic
{
void OpenGLRenderer::resizeCallback() {}

void OpenGLRenderer::enableFeatures()
{
        // todo put this in a enum
        glEnable(GL_DEPTH_TEST);
}

bool OpenGLRenderer::init(Window& window)
{
        // make sure this happens only when debug
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                IC_CORE_ERROR("Failed to initialize GLAD");
                return false;
        }

        enableFeatures();
        ic::gl::debug::setDebugOutput();
        return true;
}

void OpenGLRenderer::update(float deltaTime) {}

void OpenGLRenderer::draw()
{
        // background color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

OpenGLRenderer::OpenGLRenderer() {}

OpenGLRenderer::~OpenGLRenderer() {}

}  // namespace ic
