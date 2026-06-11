#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "defines.h"
#include "renderer/renderer.h"
#include "renderer/scene.h"
#include "renderer/camera.h"
#include "renderer/opengl/gl_shader.h"
#include "renderer/opengl/gl_skybox.h"
#include "renderer/opengl/gpu_resource_cache.h"
#include "renderer/opengl/render_command.h"
#include "renderer/opengl/render_pass.h"
#include "renderer/opengl/light_ubo.h"
#include "renderer/lighting_system.h"

#include "core/window.h"
#include "core/events/event.h"
#include "core/events/application_event.h"

namespace ic
{

class OpenGLRenderer : public IRenderer
{
public:
        OpenGLRenderer();
        ~OpenGLRenderer() override;

        OpenGLRenderer(const OpenGLRenderer &)            = delete;
        OpenGLRenderer &operator=(const OpenGLRenderer &) = delete;

        // -----------------------------------------------------------------------
        // IRenderer interface
        // -----------------------------------------------------------------------

        bool Init(Window *w) override;
        void SetScene(RenderScene *scene, Camera &editorCamera);
        void RenderFrame(float dt) override;
        void OnEvent(event &e) override;
        void ClearColor() override;
        void CleanUp() override;

private:
        // -----------------------------------------------------------------------
        // Init helpers
        // -----------------------------------------------------------------------

        void EnableFeatures();
        void CreateShader();
        void LoadAssets();
        void SetupBuffers();

        // -----------------------------------------------------------------------
        // Per-frame
        // -----------------------------------------------------------------------

        void Update(float dt);
        void Draw(float dt);

        void RenderCubeMap(Shader *cubeMapShader, GLSkybox *skybox);

        // -----------------------------------------------------------------------
        // Event handlers
        // -----------------------------------------------------------------------

        bool OnWindowResize(WindowResizedEvent &e);

        // -----------------------------------------------------------------------
        // State
        // -----------------------------------------------------------------------

        Window      *m_window        = nullptr;
        RenderScene *m_scene         = nullptr;
        bool         m_isMinimized   = false;
        Camera      *m_pEditorCamera = nullptr;

        /** TODO: make shader asset */
        Shader *m_shader        = nullptr;
        Shader *m_cubemapShader = nullptr;

        GPUResourceCache m_cache;
        RenderQueue      m_queue;
        OpaquePass       m_opaquePass;
        OutlinePass      m_outlinePass;
        TransparentPass  m_transPass;

        LightUBO    m_lightUBO;
        LightSystem m_lightSystem;

        GLSkybox m_Skybox;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif

        void ic_set_scene(ic::RenderScene *scene, Camera &editorCamera);

#ifdef __cplusplus
}
#endif

#endif  // OPENGL_RENDERER_H
