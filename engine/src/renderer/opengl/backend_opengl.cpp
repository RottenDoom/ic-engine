#include "renderer/renderer.h"
#include "opengl_renderer.h"

namespace ic
{

struct renderer::backend_context
{
        backend_context(Window& win) {}
};

struct renderer::backend_renderer
{
        OpenGLRenderer renderer;
        backend_renderer(backend_context* bc) {}
};

renderer::renderer()  = default;
renderer::~renderer() = default;

bool renderer::init(Window* w)
{
        if (m_initialized)
                return false;

        m_context = std::make_unique<backend_context>(*w);

        // if (!m_context->ctx.initialize())
        //         return false;

        m_renderer = std::make_unique<backend_renderer>(m_context.get());

        if (!m_renderer->renderer.init(*w))
                return false;

        m_initialized = true;
        return true;
}

void renderer::onEvent(event& e)
{
        m_renderer->renderer.onEvent(e);
}

void renderer::renderFrame(float dt)
{
        m_renderer->renderer.update(dt);
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
