#include "renderer/opengl/opengl_renderer.h"
#include "renderer/opengl/gl_debug.h"
#include "core/assets/asset_manager.h"
#include "core/application.h"
#include "core/ecs/entity.h"
#include "core/ecs/entity_impl.h"
#include "core/ecs/components.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace ic
{

OpenGLRenderer::OpenGLRenderer() : m_window(nullptr), m_scene(nullptr) {}

OpenGLRenderer::~OpenGLRenderer()
{
        // CleanUp() should be called explicitly before destruction,
        // but guard here in case it wasn't.
        if (m_shader)
                CleanUp();
}

bool OpenGLRenderer::Init(Window *w)
{
        IC_CORE_ASSERT(w, "OpenGLRenderer::Init -> null window");
        m_window = w;

        glfwMakeContextCurrent(m_window->GetNativeWindow());

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                IC_CORE_ERROR("OpenGLRenderer: failed to initialize GLAD");
                return false;
        }

        IC_CORE_INFO("GLFW platform: {}", glfwGetPlatform());
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

        m_lightUBO.Create();
        m_lightSystem.Init(&m_lightUBO);

        m_shader->use();
        GLuint progID = m_shader->ID;
        glUniformBlockBinding(progID, glGetUniformBlockIndex(progID, "PerFrameBlock"), 0);
        glUniformBlockBinding(progID, glGetUniformBlockIndex(progID, "LightBlock"), 1);

        m_cubemapShader->use();
        GLuint skyboxProgID = m_cubemapShader->ID;
        glUniformBlockBinding(skyboxProgID, glGetUniformBlockIndex(skyboxProgID, "PerFrameBlock"), 0);

        IC_CORE_INFO("OpenGLRenderer: initialized");
        return true;
}

void OpenGLRenderer::SetScene(RenderScene *scene, Camera &editorCamera)
{
        m_scene         = scene;
        m_pEditorCamera = &editorCamera;

        if (m_window)
        {
                LoadAssets();
                SetupBuffers();
        }
}

void OpenGLRenderer::EnableFeatures()
{
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
}

void OpenGLRenderer::CreateShader()
{
        /** TODO: Make the shaders as assets instead of this. */
        m_shader        = new Shader("shaders/opengl/pbr/pbr.vert", "shaders/opengl/pbr/pbr.frag");
        m_cubemapShader = new Shader("shaders/opengl/skybox.vert", "shaders/opengl/skybox.frag");
}

void OpenGLRenderer::LoadAssets()
{
        if (!m_scene)
                return;

        // ISSUE: multiple refcounts
        for (auto &e : m_scene->GetEntitiesWith<MeshComponent>())
        {
                MeshComponent &c     = e.GetComponent<MeshComponent>();
                Model         *model = AssetManager::Get().LoadAs<Model>(c.modelID);
                if (!model)
                        IC_CORE_WARN("LoadAssets: failed to load model {}", c.modelID);
        }
}

void OpenGLRenderer::SetupBuffers()
{
        if (!m_scene)
                return;

        for (auto &e : m_scene->GetEntitiesWith<MeshComponent>())
        {
                MeshComponent &c = e.GetComponent<MeshComponent>();
                m_cache.GetOrUpload(c.modelID, AssetManager::Get());
        }

        Skybox *skyboxAsset = m_scene->GetSkybox();
        if (!skyboxAsset || !skyboxAsset->IsLoaded())
        {
                IC_CORE_WARN("Skybox asset is not loaded!");
                return;
        }
        m_Skybox.Destroy();
        bool uploaded = m_Skybox.Upload(*skyboxAsset);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Skybox.GetCubemapID());
        GLint width = 0, height = 0, internalFmt = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFmt);
        IC_CORE_INFO("Cubemap face +X: {}x{} internalFormat: 0x{:X}", width, height, internalFmt);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        if (!uploaded)
        {
                IC_CORE_WARN("Skybox asset was not uploaded to the GPU");
        }
        skyboxAsset->Release();
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
        (void)dt;
}

void OpenGLRenderer::RenderCubeMap(Shader *cubeMapShader, GLSkybox *skybox)
{
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        cubeMapShader->use();

        GLint skyboxLoc = glGetUniformLocation(cubeMapShader->ID, "u_Skybox");
        glUniform1i(skyboxLoc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);        // unbind any 2D texture on unit 0
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);  // unbind any cubemap on unit 0

        skybox->Bind(0);

        GLint boundCubemap = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundCubemap);
        IC_CORE_INFO("RenderCubeMap -> bound cubemap on unit 0: {}", boundCubemap);

        glBindVertexArray(skybox->GetVAO());
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
}

void OpenGLRenderer::Draw(float dt)
{
        (void)dt;

        if (m_isMinimized || !m_scene || !m_shader || !m_pEditorCamera)
                return;

        ClearColor();

        // upload perframe data
        PerFrameData perFrame{};
        perFrame.view       = m_pEditorCamera->matrices.view;
        perFrame.projection = m_pEditorCamera->projection;
        perFrame.cameraPos  = glm::vec4(m_pEditorCamera->position, 0.0f);
        perFrame.time       = static_cast<float>(glfwGetTime());
        m_lightUBO.UploadPerFrame(perFrame);

        m_lightSystem.Update(m_scene, *m_pEditorCamera, glfwGetTime());
        m_lightUBO.BindAll();
        m_shader->use();

        m_queue.Clear();

        const glm::vec3 camPos = m_pEditorCamera->position;

        for (Entity &e : m_scene->GetEntitiesWith<MeshComponent, TransformComponent>())
        {
                auto &mesh      = e.GetComponent<MeshComponent>();
                auto &transform = e.GetComponent<TransformComponent>();

                GLModel *glModel = m_cache.GetOrUpload(mesh.modelID, AssetManager::Get());
                if (!glModel)
                        continue;

                std::vector<GLModel::DrawItem> items;
                glModel->CollectDrawItems(transform.GetTransformMatrix(), items);

                for (auto &item : items)
                {
                        GLMaterial *mat = nullptr;
                        if (item.materialIndex != INVALID_INDEX && glModel->Get())
                                mat = m_cache.GetMaterial(mesh.modelID, item.materialIndex, *glModel->Get());

                        const glm::vec3 center = glm::vec3(item.worldTransform[3]);
                        const float     depth  = glm::length(center - camPos);

                        m_queue.Submit({item.primitive, mat, glModel, item.worldTransform, depth});
                }
        }

        m_queue.Sort();

        m_opaquePass.Execute(m_queue.OpaqueCommands(), m_shader);
        m_outlinePass.Execute(m_queue.OutlineCommands(), m_shader);
        m_transPass.Execute(m_queue.BlendCommands(), m_shader);

        if (m_Skybox.IsReady())
                RenderCubeMap(m_cubemapShader, &m_Skybox);
}

void OpenGLRenderer::OnEvent(event &e)
{
        if (m_pEditorCamera && m_pEditorCamera->inputEnabled)
                m_pEditorCamera->OnEvent(e);

        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(OpenGLRenderer::OnWindowResize));
}

void OpenGLRenderer::ClearColor()
{
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void OpenGLRenderer::CleanUp()
{
        m_lightUBO.Destroy();
        m_cache.Clear();
        m_Skybox.Destroy();
        delete m_shader;
        delete m_cubemapShader;
        m_shader = nullptr;
}

}  // namespace ic

void ic_set_scene(ic::RenderScene *scene, Camera &editorCamera)
{
        ic::Application::Get().GetRenderer()->SetScene(scene, editorCamera);
}
