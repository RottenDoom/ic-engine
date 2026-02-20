#include "renderer/opengl/opengl_renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "renderer/opengl/gl_debug.h"
#include "core/assets/types/asset_base.h"
#include "core/assets/asset_manager.h"
#include "renderer/scene.h"

namespace ic
{

// ------------------------------------------------------------
// Resize
// ------------------------------------------------------------

bool OpenGLRenderer::onWindowResize(WindowResizedEvent &e)
{
        unsigned int width  = e.getWidth();
        unsigned int height = e.getHeight();

        if (width == 0 || height == 0)
        {
                m_IsMinimized = true;
                return false;
        }

        m_IsMinimized = false;

        glViewport(0, 0, width, height);

        if (m_scene)
                m_scene->camera.updateAspectRatio((float)((float)(width) / (float)height));

        IC_CORE_TRACE("Window Resized to: {0}x{1}", width, height);

        return false;
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void OpenGLRenderer::enableFeatures()
{
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
}

void OpenGLRenderer::createShader()
{
        shader = std::make_unique<Shader>("shaders/opengl/modelShader.vs", "shaders/opengl/modelShader.fs");
}

bool OpenGLRenderer::init(Window *w)
{
        m_window = w;
        // TODO: Put the glfw stuff in the window class
        glfwMakeContextCurrent((GLFWwindow *)m_window->getNativeWindow());

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                IC_CORE_ERROR("Failed to initialize GLAD");
                return false;
        }

        IC_CORE_INFO("Vendor:   {}", (const char *)glGetString(GL_VENDOR));
        IC_CORE_INFO("Renderer: {}", (const char *)glGetString(GL_RENDERER));
        IC_CORE_INFO("Version:  {}", (const char *)glGetString(GL_VERSION));

#if defined(DEBUG) || defined(_DEBUG)
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
                ic::gl::debug::setDebugOutput();
#endif

        glViewport(0, 0, m_window->getWidth(), m_window->getHeight());

        loadAssets();
        setupBuffers();
        enableFeatures();
        createShader();

        IC_CORE_INFO("Initialized OpenGL!");
        return true;
}

void OpenGLRenderer::setupBuffers()
{
        if (!m_scene)
                return;

        for (const auto &node : m_scene->nodes)
        {
                GUID id = node.modelID;
                IC_CORE_TRACE("Model ID: {}", id);

                if (m_gpuCache.find(id) != m_gpuCache.end())
                        continue;

                Model *model = AssetManager::Get()->getAsset<Model>(id);
                if (!model)
                        continue;

                GLModel *gpu_model = new GLModel();
                gpu_model->upload(*model);

                m_gpuCache[id] = gpu_model;

                IC_CORE_INFO("Uploaded model {} to GPU", id);
        }
}

void OpenGLRenderer::loadAssets()
{
        if (!m_scene)
                return;

        for (const auto &node : m_scene->nodes)
        {
                GUID id = node.modelID;

                // Force load into AssetManager if not already loaded
                Model *model = AssetManager::Get()->loadAs<Model>(id);

                if (!model)
                        IC_CORE_WARN("Failed to load model {}", id);
        }
}

// ------------------------------------------------------------
// Per-frame
// ------------------------------------------------------------

void OpenGLRenderer::update(float deltaTime)
{
        if (m_scene)
                m_scene->camera.onUpdate(deltaTime);
}

void OpenGLRenderer::draw(float deltaTime)
{
        if (m_IsMinimized || !m_scene)
                return;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        update(deltaTime);

        shader->use();

        shader->setMat4("u_projection", m_scene->camera.projection);
        shader->setMat4("u_view", m_scene->camera.matrices.view);

        // Render all scene objects
        for (auto &node : m_scene->nodes)
        {
                GUID id      = node.modelID;
                Model *model = AssetManager::Get()->getAsset<Model>(id);
                if (!model)
                {
                        IC_CORE_WARN("Model {} not loaded", id);
                        continue;
                }

                // Upload to GPU if not cached
                auto it = m_gpuCache.find(id);
                if (it == m_gpuCache.end())
                {
                        GLModel *gpu_model = new GLModel();
                        gpu_model->upload(*model);
                        m_gpuCache[id] = gpu_model;
                        it             = m_gpuCache.find(id);
                }

                shader->setMat4("u_model", node.transform);

                it->second->draw(*shader);
        }
}

void OpenGLRenderer::renderFrame(float dt)
{
        // START FRAME

        update(dt);
        draw(dt);

        // END FRAME
}
// ------------------------------------------------------------
// Events
// ------------------------------------------------------------

void OpenGLRenderer::onEvent(event &e)
{
        if (m_scene)
                m_scene->camera.onEvent(e);

        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(OpenGLRenderer::onWindowResize));
}

// ------------------------------------------------------------
// Cleanup
// ------------------------------------------------------------

void OpenGLRenderer::cleanUp()
{
        for (auto &[id, cache] : m_gpuCache)
        {
                cache->clearGPUMemory();
                delete cache;
        }
        m_gpuCache.clear();
}

// ------------------------------------------------------------
// Constructor / Destructor
// ------------------------------------------------------------

OpenGLRenderer::OpenGLRenderer() : m_window(nullptr), m_scene(nullptr) {}

OpenGLRenderer::~OpenGLRenderer() {}

}  // namespace ic

void ic_set_scene(Scene *scene)
{
        ic::Application::get().getRenderer()->setScene(scene);
}