#pragma once
#include "defines.h"

#include "renderer/camera.h"
#include "gl_shader.h"
#include "core/events/application_event.h"

#include "core/window.h"
#include "core/events/event.h"

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
        bool m_IsMinimized = false;

        Camera m_camera;
        Window& m_window;

        // TODO make this somewhere else
        GLuint VBO, VAO, EBO, cubeVAO;
        GLuint texture;
        GLuint lightVAO;
        std::vector<PointLight> pointLights;
        // TODO: end

        std::unique_ptr<Shader> shader;
        std::unique_ptr<Shader> lightCubeShader;

        bool onWindowResize(WindowResizedEvent& e);
        void enableFeatures();
        void createShader();

        // TODO make a buffer struct;
        void setupBuffers();

public:
        bool init();
        void update(float deltaTime);  // Make a timestep module
        void onEvent(event& e);
        void loadAssets();
        void draw(float deltaTime);
        void destroy();

        OpenGLRenderer(Window& window);
        ~OpenGLRenderer();
};
}  // namespace ic