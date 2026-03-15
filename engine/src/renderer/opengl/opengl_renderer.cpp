#include "renderer/opengl/opengl_renderer.h"
#include "renderer/opengl/gl_debug.h"
#include "core/assets/asset_manager.h"
#include "core/application.h"
#include "core/ecs/entity.h"
#include "core/ecs/entity_impl.h"
#include "core/ecs/components.h"

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
                CleanUp();
}

// ---------------------------------------------------------------------------
// IRenderer::init
// ---------------------------------------------------------------------------

bool OpenGLRenderer::Init(Window *w)
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

        EnableFeatures();
        CreateShader();

        LoadAssets();
        SetupBuffers();

        IC_CORE_INFO("OpenGLRenderer: initialized");
        return true;
}

// ---------------------------------------------------------------------------
// IRenderer::setScene
// ---------------------------------------------------------------------------

void OpenGLRenderer::SetScene(RenderScene *scene)
{
        m_scene = scene;
        // TODO: Remove the camera from here and make an entity out of it
        m_scene->defaultCamera = createCamera(Camera::CameraType::firstperson, glm::vec3(0.0f, 0.0f, 0.0f));

        // If the renderer is already initialized, load and upload the new scene.
        if (m_window)
        {
                LoadAssets();
                SetupBuffers();
        }
}

// ---------------------------------------------------------------------------
// Init helpers
// ---------------------------------------------------------------------------

void OpenGLRenderer::EnableFeatures()
{
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        // glFrontFace(GL_CCW);
}

void OpenGLRenderer::CreateShader()
{
        m_shader = new Shader("shaders/opengl/modelShader.vs", "shaders/opengl/modelShader.fs");
}

void OpenGLRenderer::LoadAssets()
{
        if (!m_scene)
                return;

        /** Just for the sake of making sure
         * 1. m_scene get all the entities with meshcomponent
         * 2. for each component load the model or material whatever from them
         * 3. To keep this shit loaded
         */

        std::vector<Entity> mesh_entities = m_scene->GetEntitiesWith<MeshComponent>();
        for (auto &e : mesh_entities)
        {
                MeshComponent &c     = e.GetComponent<MeshComponent>();
                Model         *model = AssetManager::Get().loadAs<Model>(c.modelID);

                if (!model)
                {
                        IC_CORE_WARN("Failed loading model {}", c.modelID);
                }
        }
}

void OpenGLRenderer::SetupBuffers()
{
        if (!m_scene)
                return;

        std::vector<Entity> mesh_entities = m_scene->GetEntitiesWith<MeshComponent>();
        for (auto &e : mesh_entities)
        {
                MeshComponent &c = e.GetComponent<MeshComponent>();
                if (m_gpuCache.count(c.modelID))
                        continue;
                UploadModel(c.modelID);
        }
}

GLModel *OpenGLRenderer::UploadModel(GUID id)
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

GLModel *OpenGLRenderer::GetOrUpload(GUID id)
{
        auto it = m_gpuCache.find(id);

        if (it != m_gpuCache.end())
                return it->second;

        return UploadModel(id);
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void OpenGLRenderer::RenderFrame(float dt)
{
        Update(dt);
        Draw(dt);
}

void OpenGLRenderer::Update(float dt)
{
        // Entity Camera?
        if (m_scene)
                m_scene->defaultCamera.onUpdate(dt);
}

void OpenGLRenderer::Draw(float dt)
{
        (void)dt;

        if (m_isMinimized || !m_scene || !m_shader)
                return;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->use();
        /** TODO: Fix these */
        m_shader->setMat4("u_projection", m_scene->defaultCamera.projection);
        m_shader->setMat4("u_view", m_scene->defaultCamera.matrices.view);

        auto meshEntities = m_scene->GetEntitiesWith<MeshComponent, TransformComponent>();

        for (Entity &e : meshEntities)
        {
                auto &mesh      = e.GetComponent<MeshComponent>();
                auto &transform = e.GetComponent<TransformComponent>();

                GLModel *glModel = GetOrUpload(mesh.modelID);
                if (!glModel)
                        continue;

                m_shader->setMat4("u_model", transform.GetTransformMatrix());
                glModel->draw(m_shader);
        }
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void OpenGLRenderer::OnEvent(event &e)
{
        if (m_scene)
                m_scene->defaultCamera.onEvent(e);

        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(OpenGLRenderer::OnWindowResize));
}

bool OpenGLRenderer::OnWindowResize(WindowResizedEvent &e)
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

void OpenGLRenderer::CleanUp()
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
        ic::Application::Get().GetRenderer()->SetScene(scene);
}
