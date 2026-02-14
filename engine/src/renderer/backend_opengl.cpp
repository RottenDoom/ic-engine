#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"
// #include "renderer/vulkan/vulkan_renderer.h"  // When you add Vulkan

namespace ic
{

struct Renderer::backend_context
{
        backend_context(Window &w) {}
};

struct Renderer::backend_renderer
{
        OpenGLRenderer renderer;
        // VulkanRenderer renderer;  // Or switch based on config

        backend_renderer(Window &w) : renderer(w) {}
};

Renderer::Renderer()  = default;
Renderer::~Renderer() = default;

bool Renderer::init(Window *w)
{
        if (m_initialized)
                return false;

        m_context  = std::make_unique<backend_context>(*w);
        m_renderer = std::make_unique<backend_renderer>(*w);

        if (!m_renderer->renderer.init())
                return false;

        m_initialized = true;
        IC_CORE_INFO("Renderer Initialized");
        return true;
}

void Renderer::onEvent(event &e)
{
        m_renderer->renderer.onEvent(e);
}

void Renderer::renderFrame(float dt)
{
        m_renderer->renderer.draw(dt);
}

void Renderer::cleanUp()
{
        if (!m_initialized)
                return;

        m_renderer->renderer.destroy();
        m_initialized = false;
}

// Factory functions (implementation in engine-renderer)
IRenderer *createRenderer()
{
        // MEMORY STUFF HERE
        return new Renderer();
}

void destroyRenderer(IRenderer *renderer)
{
        delete renderer;
}

}  // namespace ic