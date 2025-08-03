#include "renderer.h"
#include "defines.h"

namespace ic
{

    void renderer::init(GLFWwindow *window)
    {
        if (!m_context.init()) {
            IC_CORE_FATAL_IF(!m_initialized, "Renderer not initialized");
            return;
        }
        m_initialized = true;
        IC_CORE_INFO("Renderer Initialized");
    }

    void renderer::renderFrame()
    {
        IC_CORE_FATAL_IF(!m_initialized, "Render called before Initialization!");

        // IC_INFO("Renderer Called!");
    }

    void renderer::cleanUp()
    {
        if (!m_initialized) return;
        m_initialized = false;
        m_context.cleanUp();
        IC_CORE_INFO("Renderer Cleaned Up!");
    }

} // namespace ic
