#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "defines.h"
#include "renderer/camera.h"
#include "renderer/scene.h"
#include "renderer/renderer.h"

#include "gl_shader.h"
#include "gl_model.h"
#include "core/gltf_loader.h"
#include "core/application.h"

#include "core/window.h"
#include "core/events/event.h"
#include "core/events/application_event.h"

struct PointLight
{
        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float constant;
        float linear;
        float quadratic;
};
namespace ic
{

class OpenGLRenderer : public IRenderer
{
private:
        bool m_IsMinimized = false;  //[TODO] handle minimized

        Window *m_window;

        /** TODO: Make a scene class */
        Scene *m_scene;
        std::unordered_map<GUID, GLModel *> m_gpuCache;

        /** TODO: Lighting class */
        // std::vector<PointLight> pointLights;

        std::unique_ptr<Shader> shader;
        // std::unique_ptr<Shader> lightCubeShader;

        bool onWindowResize(WindowResizedEvent &e);
        void enableFeatures();
        void createShader();

        // TODO make a buffer struct;
        void setupBuffers();

public:
        bool init(Window *w) override;
        void onEvent(event &e) override;
        void renderFrame(float dt) override;
        void cleanUp() override;
        void setScene(Scene *scene) override { m_scene = scene; }

        void update(float deltaTime);  // Make a timestep module
        void loadAssets();
        void draw(float deltaTime);

        OpenGLRenderer();
        ~OpenGLRenderer();
};

}  // namespace ic

#endif
