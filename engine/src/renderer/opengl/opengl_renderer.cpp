#include "opengl_renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "gl_debug.h"

// TODO vertices are not supposed to be here.
float vertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f, -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f, 0.0f,

    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f, 1.0f,
    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f, 1.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f, 1.0f};

glm::vec3 cubePositions[]       = {glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(2.0f, 5.0f, -15.0f),
                                   glm::vec3(-1.5f, -2.2f, -2.5f),
                                   glm::vec3(-3.8f, -2.0f, -12.3f),
                                   glm::vec3(2.4f, -0.4f, -3.5f),
                                   glm::vec3(-1.7f, 3.0f, -7.5f),
                                   glm::vec3(1.3f, -2.0f, -2.5f),
                                   glm::vec3(1.5f, 2.0f, -2.5f),
                                   glm::vec3(1.5f, 0.2f, -1.5f),
                                   glm::vec3(-1.3f, 1.0f, -1.5f)};

glm::vec3 pointLightPositions[] = {glm::vec3(0.7f, 0.2f, 2.0f),
                                   glm::vec3(2.3f, -3.3f, -4.0f),
                                   glm::vec3(-4.0f, 2.0f, -12.0f),
                                   glm::vec3(0.0f, 0.0f, -3.0f)};

namespace ic
{

bool OpenGLRenderer::onWindowResize(WindowResizedEvent& e)
{
        unsigned int width  = e.getWidth();
        unsigned int height = e.getHeight();

        if (width == 0 || height == 0)
        {
                m_IsMinimized = true;  // flag to stop the Render Loop
                return false;          // Let other systems know we are minimized
        }

        m_IsMinimized = false;

        // 2. Update OpenGL state
        glViewport(0, 0, width, height);

        // [TODO]
        // m_Camera.setViewportSize(width, height);

        IC_CORE_TRACE("Window Resized to: {0}x{1}", width, height);

        return false;  // Return false so other layers (like UI) can also resize
}

void OpenGLRenderer::enableFeatures()
{
        // todo put this in a enum
        glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::createShader()
{
        shader          = std::make_unique<Shader>("shaders/opengl/shader.vs", "shaders/opengl/shader.fs");
        lightCubeShader = std::make_unique<Shader>("shaders/opengl/lightShader.vs", "shaders/opengl/lightShader.fs");
}

void OpenGLRenderer::setupBuffers()
{
        // [TODO IMP]: get this logic somwhere else
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texcoords vectices
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glGenVertexArrays(1, &lightVAO);
        glBindVertexArray(lightVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        /** stride */
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        IC_CORE_INFO("[TODO: Remove this] The array pointers are created!");
}

bool OpenGLRenderer::init()
{
        glfwMakeContextCurrent((GLFWwindow*)m_window.getNativeWindow());

        // Initialize GL function pointers before making GL calls (glGetIntegerv, etc.)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
                IC_CORE_ERROR("Failed to initialize GLAD");
                return false;
        }

#if defined(DEBUG) || defined(_DEBUG)
        // Note: GLFW_OPENGL_DEBUG_CONTEXT must be set before window creation (glfwCreateWindow).
        // If you want a debug context, set the hint in the Window class before creating the GLFW window.
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
                ic::gl::debug::setDebugOutput();
        }
#endif

        // setviewport function
        int width  = m_window.getWidth();
        int height = m_window.getHeight();
        glViewport(0, 0, width, height);

        enableFeatures();
        createShader();
        loadAssets();
        setupBuffers();

        IC_CORE_INFO("Initialized OpenGL!");
        return true;
}

void OpenGLRenderer::update(float deltaTime)
{
        m_camera.onUpdate(deltaTime);
}

void OpenGLRenderer::onEvent(event& e)
{
        m_camera.onEvent(e);
        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizedEvent>(BIND_EVENT(OpenGLRenderer::onWindowResize));
}

void OpenGLRenderer::loadAssets()
{
        /** TODO: This is only for testing remove this */
        GLTFLoader loader;
        if (!loader.loadModel("tree_house/scene.gltf", &model))
        {
                IC_CORE_WARN("Model did not load bruh!");
                return;
        }
        IC_CORE_INFO("Loaded The model somehow I need the name of the model as well here or UUID");
}

void OpenGLRenderer::draw(float deltaTime)
{
        // background color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        update(deltaTime);

        shader->use();

        /** Turn off the lights for now but will add a toggle using ImGUI */
        // directional light
        shader->setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        shader->setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        shader->setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        shader->setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        for (int i = 0; i < pointLights.size(); i++)
        {
                std::string index = "pointLights[" + std::to_string(i) + "]";

                shader->setVec3(index + ".position", pointLights[i].position);
                shader->setVec3(index + ".ambient", pointLights[i].ambient);
                shader->setVec3(index + ".diffuse", pointLights[i].diffuse);
                shader->setVec3(index + ".specular", pointLights[i].specular);
                shader->setFloat(index + ".constant", pointLights[i].constant);
                shader->setFloat(index + ".linear", pointLights[i].linear);
                shader->setFloat(index + ".quadratic", pointLights[i].quadratic);
        }

        shader->setVec3("spotLight.position", m_camera.position);
        // shader->setVec3("spotLight.direction", m_camera.front);
        shader->setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        shader->setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        shader->setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        shader->setFloat("spotLight.constant", 1.0f);
        shader->setFloat("spotLight.linear", 0.09f);
        shader->setFloat("spotLight.quadratic", 0.032f);
        shader->setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        shader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        shader->setFloat("time", glfwGetTime());  // TODO: make a time module

        shader->setMat4("projection", m_camera.projection);
        shader->setMat4("view", m_camera.matrices.view);

        glm::mat4 model = glm::mat4(1.0f);
        model           = glm::translate(model,
                               glm::vec3(0.0f, 0.0f, 0.0f));  // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));  // it's a bit too big for our scene, so scale it down
        shader->setMat4("model", model);

        // bind should be in the bind functions
        glBindVertexArray(cubeVAO);
        for (unsigned int i = 0; i < 10; i++)
        {
                glm::mat4 model = glm::mat4(1.0f);
                model           = glm::translate(model, cubePositions[i]);
                float angle     = 20.0f * i;
                model           = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                shader->setMat4("model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        lightCubeShader->use();
        lightCubeShader->setMat4("projection", m_camera.projection);
        lightCubeShader->setMat4("view", m_camera.matrices.view);

        glBindVertexArray(lightVAO);
        for (size_t i = 0; i < pointLights.size(); i++)
        {
                model = glm::mat4(1.0f);
                model = glm::translate(model, pointLightPositions[i]);
                model = glm::scale(model, glm::vec3(0.2f));

                lightCubeShader->setMat4("model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
        }
}

void OpenGLRenderer::destroy()
{
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteVertexArrays(1, &lightVAO);
        glDeleteBuffers(1, &VBO);
}

OpenGLRenderer::OpenGLRenderer(Window& window) : m_window(window)
{
        m_camera.type = Camera::CameraType::lookat;
        m_camera.setPosition(glm::vec3(0.0f, 0.0f, -1.0f));
        m_camera.setViewDirection(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f));  // look into +ve z axiz
        // m_camera.setRotationSpeed(0.5f);
        m_camera.setPerspectiveProjection(45.0f, (float)m_window.getWidth() / (float)m_window.getHeight(), 0.1f, 256.0f);
}

OpenGLRenderer::~OpenGLRenderer() {}

}  // namespace ic
