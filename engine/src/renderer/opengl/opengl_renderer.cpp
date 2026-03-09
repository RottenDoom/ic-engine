#include "renderer/opengl/opengl_renderer.h"
#include "renderer/opengl/gl_debug.h"
#include "core/assets/asset_manager.h"
#include "core/application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

/**
 * opengl_renderer.cpp
 *
 * TODO:
 * 1. Fix the new refactored pipeline base renderer with the asset manager
 * 2. Lighting system with proper things inplace
 * 3. UI system both for Editor and Application
 *
 */

namespace ic
{

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

OpenGLRenderer::OpenGLRenderer() : m_window(nullptr), m_scene(nullptr) {}

OpenGLRenderer::~OpenGLRenderer()
{
        // cleanUp() should be called explicitly before destruction,
        // but guard here in case it wasn't.
        if (!m_gpuCache.empty())
                cleanUp();
}

// ---------------------------------------------------------------------------
// IRenderer::init
// ---------------------------------------------------------------------------

bool OpenGLRenderer::init(Window *w)
{
        IC_CORE_ASSERT(w, "OpenGLRenderer::init -> null window");
        m_window = w;

        glfwMakeContextCurrent(static_cast<GLFWwindow *>(m_window->getNativeWindow()));

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
                IC_CORE_ERROR("OpenGLRenderer: failed to initialize GLAD");
                return false;
        }

        IC_CORE_INFO("GL Vendor:   {}", reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
        IC_CORE_INFO("GL Renderer: {}", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
        IC_CORE_INFO("GL Version:  {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

#if defined(DEBUG) || defined(_DEBUG)
        {
                int flags = 0;
                glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
                if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
                        ic::gl::debug::setDebugOutput();
        }
#endif

        glViewport(0, 0, static_cast<GLsizei>(m_window->getWidth()), static_cast<GLsizei>(m_window->getHeight()));

        enableFeatures();
        createShader();

        // Asset loading and GPU upload require a scene to be set.
        // If no scene is set yet these are no-ops; call setupBuffers()
        // again after setScene() if needed.
        loadAssets();
        setupBuffers();

        IC_CORE_INFO("OpenGLRenderer: initialized");
        return true;
}

// ---------------------------------------------------------------------------
// IRenderer::setScene
// ---------------------------------------------------------------------------

void OpenGLRenderer::setScene(ICScene *scene)
{
        m_scene = scene;

        // If the renderer is already initialized, load and upload the new scene.
        if (m_window)
        {
                loadAssets();
                setupBuffers();
        }
}

// ---------------------------------------------------------------------------
// Init helpers
// ---------------------------------------------------------------------------

void OpenGLRenderer::enableFeatures()
{
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
}

void OpenGLRenderer::createShader()
{
        m_shader = new Shader("shaders/opengl/modelShader.vs", "shaders/opengl/modelShader.fs");
}

void OpenGLRenderer::loadAssets()
{
        if (!m_scene)
                return;

        for (const auto &node : m_scene->nodes)
        {
                if (node.modelPath.empty())
                {
                        IC_CORE_WARN("OpenGLRenderer::loadAssets -> node has no modelPath, skipping");
                        continue;
                }

                Model *model = AssetManager::Get()->loadAs<Model>(node.modelID);
                if (!model)
                        IC_CORE_WARN("OpenGLRenderer::loadAssets -> failed to load model '{}'", node.modelPath);
        }
}

void OpenGLRenderer::setupBuffers()
{
        if (!m_scene)
                return;

        for (const auto &node : m_scene->nodes)
        {
                GUID id = node.modelID;

                // Skip if already uploaded
                if (m_gpuCache.count(id))
                        continue;

                uploadModel(id);
        }
}

GLModel *OpenGLRenderer::uploadModel(GUID id)
{
        Model *model = AssetManager::Get()->getAsset<Model>(id);
        if (!model)
        {
                IC_CORE_WARN("OpenGLRenderer::uploadModel -> model {} not in AssetManager", id);
                return nullptr;
        }

        if (!model->isCPUReady())
        {
                IC_CORE_WARN("OpenGLRenderer::uploadModel -> model {} not CPUReady (state={})",
                             id,
                             static_cast<int>(model->getState()));
                return nullptr;
        }

        auto glModel = std::make_unique<GLModel>();
        glModel->upload(*model);

        GLModel *raw = glModel.get();
        m_gpuCache.emplace(id, std::move(glModel));

        IC_CORE_INFO("OpenGLRenderer: uploaded model {} to GPU", id);
        return raw;
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void OpenGLRenderer::renderFrame(float dt)
{
        update(dt);
        draw(dt);
}

void OpenGLRenderer::update(float dt)
{
        if (m_scene)
                m_scene->camera.onUpdate(dt);
}

void OpenGLRenderer::draw(float dt)
{
        (void)dt;

        if (m_isMinimized || !m_scene || !m_shader)
                return;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->use();
        m_shader->setMat4("u_projection", m_scene->camera.projection);
        m_shader->setMat4("u_view", m_scene->camera.matrices.view);

        for (const auto &node : m_scene->nodes)
        {
                GUID id = node.modelID;

                auto it = m_gpuCache.find(id);
                if (it == m_gpuCache.end())
                {
                        IC_CORE_WARN("OpenGLRenderer::draw -> model {} not in GPU cache, skipping", id);
                        continue;
                }

                m_shader->setMat4("u_model", node.transform);
                it->second->draw(m_shader);
        }
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void OpenGLRenderer::onEvent(event &e)
{
        if (m_scene)
                m_scene->camera.onEvent(e);

        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(OpenGLRenderer::onWindowResize));
}

bool OpenGLRenderer::onWindowResize(WindowResizedEvent &e)
{
        const unsigned int w = e.getWidth();
        const unsigned int h = e.getHeight();

        if (w == 0 || h == 0)
        {
                m_isMinimized = true;
                return false;
        }

        m_isMinimized = false;
        glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));

        IC_CORE_TRACE("OpenGLRenderer: window resized to {}x{}", w, h);
        return false;
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

void OpenGLRenderer::cleanUp()
{
        // unique_ptr calls GLModel destructor, which should call clearGPUMemory().
        // If GLModel doesn't have a destructor doing that, add one.
        for (auto &[id, glModel] : m_gpuCache)
                glModel->clearGPUMemory();

        m_gpuCache.clear();
        delete m_shader;
        m_shader = nullptr;
}

}  // namespace ic

// ---------------------------------------------------------------------------
// C-linkage scene setter -> allows script/C layers to set the scene without
// pulling in C++ renderer headers.
// ---------------------------------------------------------------------------

extern "C" void ic_set_scene(ICScene *scene)
{
        ic::Application::get().getRenderer()->setScene(scene);
}