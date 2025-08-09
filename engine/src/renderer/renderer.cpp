#include "renderer.h"
#include "defines.h"

namespace ic
{

    bool renderer::init(GLFWwindow *window)
    {
        if (m_initialized) {
            IC_CORE_WARN("Renderer already initialized");
            return false;
        }

        m_context = std::make_unique<vulkan_context>();

        device_requirements requirements;
        if (!m_context->initialize(window, true, requirements)) {
            IC_CORE_ERROR("Failed to initialize renderer!");
            return false;
        }
        m_initialized = true;
        IC_CORE_INFO("Renderer Initialized");
        return true;
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
        m_context->cleanUp();
        IC_CORE_INFO("Renderer Cleaned Up!");
    }

} // namespace ic
