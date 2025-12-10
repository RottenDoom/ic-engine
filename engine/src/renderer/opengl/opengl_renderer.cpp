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
        // Must be called BEFORE gladLoadGLLoader
        glfwMakeContextCurrent((GLFWwindow*)window.getNativeWindow());

#if defined(DEBUG) || defined(_DEBUG)
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
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

void OpenGLRenderer::onEvent(event& e) {}

void OpenGLRenderer::draw()
{
        // background color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::destroy() {}

OpenGLRenderer::OpenGLRenderer() {}

OpenGLRenderer::~OpenGLRenderer() {}

}  // namespace ic
