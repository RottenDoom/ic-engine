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

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
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

        loadAssets();
        setupBuffers();

        IC_CORE_INFO("OpenGLRenderer: initialized");
        return true;
}

// ---------------------------------------------------------------------------
// IRenderer::setScene
// ---------------------------------------------------------------------------

void OpenGLRenderer::setScene(RenderScene *scene)
{
        m_scene = scene;
        // TODO: Remove the camera from here and make an entity out of it
        m_scene->defaultCamera = createCamera(Camera::CameraType::firstperson, glm::vec3(0.0f, 0.0f, 0.0f));

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
        // glFrontFace(GL_CCW);
}

void OpenGLRenderer::createShader()
{
        m_shader = new Shader("shaders/opengl/modelShader.vs", "shaders/opengl/modelShader.fs");
}

void OpenGLRenderer::loadAssets()
{
        if (!m_scene)
                return;

        // for (auto &[entity, mesh] : m_scene->meshes())
        // {
        //         Model *model = AssetManager::Get()->loadAs<Model>(mesh.modelID);

        //         if (!model)
        //         {
        //                 IC_CORE_WARN("Failed loading model {}", mesh.modelID);
        //         }
        // }
}

void OpenGLRenderer::setupBuffers()
{
        if (!m_scene)
                return;

        // for (auto &[entity, mesh] : m_scene->meshes())
        // {
        //         if (m_gpuCache.count(mesh.modelID))
        //                 continue;

        //         uploadModel(mesh.modelID);
        // }
}

GLModel *OpenGLRenderer::uploadModel(GUID id)
{
        Model *model = AssetManager::Get().getAsset<Model>(id);
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

        auto glModel = new GLModel();
        glModel->upload(*model);

        m_gpuCache[id] = glModel;

        IC_CORE_INFO("OpenGLRenderer: uploaded model {} to GPU", id);
        return glModel;
}

GLModel *OpenGLRenderer::getOrUpload(GUID id)
{
        auto it = m_gpuCache.find(id);

        if (it != m_gpuCache.end())
                return it->second;

        return uploadModel(id);
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
        // Entity Camera?
        if (m_scene)
                m_scene->defaultCamera.onUpdate(dt);
}

void OpenGLRenderer::draw(float dt)
{
        (void)dt;

        if (m_isMinimized || !m_scene || !m_shader)
                return;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->use();
        /** TODO: Fix these */
        // m_shader->setMat4("u_projection", m_scene->camera.projection);
        // m_shader->setMat4("u_view", m_scene->camera.matrices.view);

        // auto &meshes     = m_scene->meshes();
        // auto &transforms = m_scene->transforms();

        // for (auto &[entity, mesh] : meshes)
        // {
        //         auto tIt = transforms.find(entity);

        //         if (tIt == transforms.end())
        //                 continue;

        //         GLModel *glModel = getOrUpload(mesh.modelID);

        //         if (!glModel)
        //                 continue;

        //         glm::mat4 modelMat = tIt->second.matrix();

        //         m_shader->setMat4("u_model", modelMat);

        //         glModel->draw(m_shader);
        // }
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void OpenGLRenderer::onEvent(event &e)
{
        if (m_scene)
                m_scene->defaultCamera.onEvent(e);

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
        for (auto &it : m_gpuCache)
        {
                it.second->clearGPUMemory();
                delete it.second;
        }

        m_gpuCache.clear();
        delete m_shader;
        m_shader = nullptr;
}

}  // namespace ic

void ic_set_scene(ic::RenderScene *scene)
{
        ic::Application::Get().GetRenderer()->setScene(scene);
}
