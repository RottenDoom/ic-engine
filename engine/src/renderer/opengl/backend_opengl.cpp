#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"

namespace ic
{

struct renderer::backend_context
{
        backend_context(Window &w) {}
};

struct renderer::backend_renderer
{
        OpenGLRenderer renderer;
        backend_renderer(Window &w) : renderer(w) {}
};

renderer::renderer()  = default;
renderer::~renderer() = default;

bool renderer::init(Window *w)
{
        if (m_initialized)
                return false;

        m_context = std::make_unique<backend_context>(*w);

        // if (!m_context->ctx.initialize())
        //         return false;

        m_renderer = std::make_unique<backend_renderer>(*w);

        if (!m_renderer->renderer.init())
                return false;

        m_initialized = true;
        IC_CORE_INFO("Renderer Initialized");
        return true;
}

void renderer::onEvent(event &e)
{
        m_renderer->renderer.onEvent(e);
}

void renderer::renderFrame(float dt)
{
        m_renderer->renderer.draw(dt);
}

void renderer::cleanUp()
{
        if (!m_initialized)
                return;
        m_renderer->renderer.destroy();
        // m_context->ctx.cleanUp();
        m_initialized = false;
}

}  // namespace ic
