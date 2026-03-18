#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "defines.h"
#include "renderer/renderer.h"
#include "renderer/scene.h"
#include "renderer/camera.h"
#include "renderer/opengl/gl_shader.h"
#include "renderer/opengl/gl_model.h"
#include "core/window.h"
#include "core/events/event.h"
#include "core/events/application_event.h"

#include <memory>
#include <unordered_map>

/**
 * opengl_renderer.h
 *
 * OpenGL backend implementation of IRenderer.
 *
 *
 * GPU upload policy:
 *   setupBuffers() is called once during init() and uploads all scene models.
 *   draw() never uploads -> if a model isn't in m_gpuCache it logs a warning
 *   and skips rather than stalling the render thread mid-frame.
 * 
 * TODO: connect the transform system and other things to actual model matrices
 * TODO: start with a basic imgui demo base. and start with a basic component window for tranforms.
 * TODO: Add more asset types and more models and fix the material system.
 * TODO: Big goal: Scene Graph.
 */

namespace ic
{

// ---------------------------------------------------------------------------
// PointLight -> kept here until a proper lighting system exists
// ---------------------------------------------------------------------------

struct PointLight
{
        glm::vec3 position  = glm::vec3(0.0f);
        glm::vec3 ambient   = glm::vec3(0.1f);
        glm::vec3 diffuse   = glm::vec3(1.0f);
        glm::vec3 specular  = glm::vec3(1.0f);
        float     constant  = 1.0f;
        float     linear    = 0.09f;
        float     quadratic = 0.032f;
};

// ---------------------------------------------------------------------------
// OpenGLRenderer
// ---------------------------------------------------------------------------

class OpenGLRenderer : public IRenderer
{
public:
        OpenGLRenderer();
        ~OpenGLRenderer() override;

        // Non-copyable -> owns GPU resources
        OpenGLRenderer(const OpenGLRenderer &)            = delete;
        OpenGLRenderer &operator=(const OpenGLRenderer &) = delete;

        // -----------------------------------------------------------------------
        // IRenderer interface
        // -----------------------------------------------------------------------

        bool Init(Window *w) override;

        void SetScene(RenderScene *scene);

        void RenderFrame(float dt) override;

        void OnEvent(event &e) override;

        void CleanUp() override;

private:
        // -----------------------------------------------------------------------
        // Init helpers -> called once from init()
        // -----------------------------------------------------------------------

        /** Enable depth test, face culling, etc. */
        void EnableFeatures();

        /** Compile and link the PBR model shader. */
        void CreateShader();

        /**
         * Load all model assets referenced by the current scene into AssetManager.
         * Must be called before setupBuffers().
         */
        void LoadAssets();

        /**
         * Upload all loaded scene models to the GPU.
         * Populates m_gpuCache. Called once after loadAssets().
         */
        void SetupBuffers();

        // -----------------------------------------------------------------------
        // Per-frame helpers -> called from renderFrame()
        // -----------------------------------------------------------------------

        void Update(float dt);
        void Draw(float dt);

        // -----------------------------------------------------------------------
        // Event handlers
        // -----------------------------------------------------------------------

        bool OnWindowResize(WindowResizedEvent &e);

        // -----------------------------------------------------------------------
        // Internal: upload a single model to GPU and cache it
        // Returns the cached GLModel or nullptr on failure.
        // -----------------------------------------------------------------------
        GLModel *UploadModel(GUID id);
        GLModel *GetOrUpload(GUID id);

        // -----------------------------------------------------------------------
        // State
        // -----------------------------------------------------------------------
private:
        Window      *m_window      = nullptr;
        RenderScene *m_scene       = nullptr;  // See into this and more of this
        bool         m_isMinimized = false;

        // GPU cache -> one GLModel per unique model GUID..
        std::unordered_map<GUID, GLModel *> m_gpuCache;

        Shader *m_shader = nullptr;

        // TODO: lighting system
        // std::vector<PointLight> m_pointLights;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif

        void ic_set_scene(ic::RenderScene *scene);

#ifdef __cplusplus
}
#endif

#endif  // OPENGL_RENDERER_H