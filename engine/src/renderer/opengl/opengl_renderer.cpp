#include "opengl_renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl_debug.h"
namespace ic
{

bool OpenGLRenderer::onWindowResize(WindowResizedEvent& e)
{
        unsigned int width  = e.getWidth();
        unsigned int height = e.getHeight();

        if (width == 0 || height == 0)
        {
                m_IsMinimized = true;  // flag to stop the Render Loop
                return false;          // Let other systems know we are minimized
        }

        m_IsMinimized = false;

        // 2. Update OpenGL state
        glViewport(0, 0, width, height);

        // [TODO]
        // m_Camera.setViewportSize(width, height);

        IC_CORE_TRACE("Window Resized to: {0}x{1}", width, height);

        return false;  // Return false so other layers (like UI) can also resize
}

void OpenGLRenderer::enableFeatures()
{
        // todo put this in a enum
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);  // or GL_CW if needed
}

void OpenGLRenderer::createShader()
{
        shader = std::make_unique<Shader>("shaders/opengl/testShader.vs", "shaders/opengl/testShader.fs");

        /** TODO: Important */
        // lightCubeShader = std::make_unique<Shader>("shaders/opengl/lightShader.vs", "shaders/opengl/lightShader.fs");
}

void OpenGLRenderer::setupBuffers()
{
        gpuHandle.upload(model);
}

bool OpenGLRenderer::init()
{
        glfwMakeContextCurrent((GLFWwindow*)m_window.getNativeWindow());

        // Initialize GL function pointers before making GL calls (glGetIntegerv, etc.)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                IC_CORE_ERROR("Failed to initialize GLAD");
                return false;
        }

        /** TODO: This is giving me a hint for creating my own logger. */
        std::cout << "GL Version: " << glGetString(GL_VERSION) << std::endl;
        // IC_CORE_INFO("GL VERSION: {}", static_cast<const unsigned char*>(glGetString(GL_VERSION)));

#if defined(DEBUG) || defined(_DEBUG)
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
                ic::gl::debug::setDebugOutput();
        }
#endif

        // setviewport function
        int width  = m_window.getWidth();
        int height = m_window.getHeight();
        glViewport(0, 0, width, height);

        enableFeatures();
        createShader();
        loadAssets();
        setupBuffers();

        IC_CORE_INFO("Initialized OpenGL!");
        return true;
}

void OpenGLRenderer::update(float deltaTime)
{
        m_camera.onUpdate(deltaTime);
}

void OpenGLRenderer::onEvent(event& e)
{
        m_camera.onEvent(e);
        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(OpenGLRenderer::onWindowResize));
}

void OpenGLRenderer::loadAssets()
{
        /** TODO: This is only for testing remove this */
        GLTFLoader loader;
        if (!loader.loadModel("tree_house/scene.gltf", &model))
        {
                IC_CORE_WARN("Model did not load bruh!");
                return;
        }
        IC_CORE_INFO("Loaded the Model successfully");
}

void OpenGLRenderer::draw(float deltaTime)
{
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // background color
        update(deltaTime); /** TODO: look into this */

        shader->use();

        shader->setMat4("projection", m_camera.projection);
        shader->setMat4("view", m_camera.matrices.view);

        gpuHandle.draw(*shader); /** TODO: look into this */
}

void OpenGLRenderer::destroy() {}

OpenGLRenderer::OpenGLRenderer(Window& window) : m_window(window)
{
        m_camera.type = Camera::CameraType::firstperson;
        m_camera.setViewDirection(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // m_camera.setRotationSpeed(0.5f);
        m_camera.setPerspectiveProjection(45.0f, (float)m_window.getWidth() / (float)m_window.getHeight(), 0.1f, 256.0f);
}

OpenGLRenderer::~OpenGLRenderer() {}

}  // namespace ic
