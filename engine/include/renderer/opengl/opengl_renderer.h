#pragma once

#include "defines.h"
#include "renderer/camera.h"

#include "gl_shader.h"
#include "gl_model.h"
#include "core/gltf_loader.h"

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
class OpenGLRenderer
{
private:
        bool m_IsMinimized = false;  //[TODO] handle minimized

        Camera m_camera;
        Window &m_window;

        /** TODO: Make these a handle library */
        Model model;
        GLModel gpuHandle;

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
        bool init();
        void update(float deltaTime);  // Make a timestep module
        void onEvent(event &e);
        void loadAssets();
        void draw(float deltaTime);
        void destroy();

        OpenGLRenderer(Window &window);
        ~OpenGLRenderer();
};
}  // namespace ic