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

        bool init(Window *w) override;

        void setScene(RenderScene *scene);

        void renderFrame(float dt) override;

        void onEvent(event &e) override;

        void cleanUp() override;

private:
        // -----------------------------------------------------------------------
        // Init helpers -> called once from init()
        // -----------------------------------------------------------------------

        /** Enable depth test, face culling, etc. */
        void enableFeatures();

        /** Compile and link the PBR model shader. */
        void createShader();

        /**
         * Load all model assets referenced by the current scene into AssetManager.
         * Must be called before setupBuffers().
         */
        void loadAssets();

        /**
         * Upload all loaded scene models to the GPU.
         * Populates m_gpuCache. Called once after loadAssets().
         */
        void setupBuffers();

        // -----------------------------------------------------------------------
        // Per-frame helpers -> called from renderFrame()
        // -----------------------------------------------------------------------

        void update(float dt);
        void draw(float dt);

        // -----------------------------------------------------------------------
        // Event handlers
        // -----------------------------------------------------------------------

        bool onWindowResize(WindowResizedEvent &e);

        // -----------------------------------------------------------------------
        // Internal: upload a single model to GPU and cache it
        // Returns the cached GLModel or nullptr on failure.
        // -----------------------------------------------------------------------
        GLModel *uploadModel(GUID id);
        GLModel *getOrUpload(GUID id);

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