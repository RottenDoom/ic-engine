#include "renderer.h"

namespace ic
{

        bool renderer::init(GLFWwindow* window)
        {
                if (m_initialized)
                {
                        IC_CORE_WARN("Renderer already initialized");
                        return false;
                }

                m_context = std::make_unique<vulkan_context>();

                if (!m_context->initialize(window, true))
                {
                        IC_CORE_ERROR("Failed to initialize context!");
                        return false;
                }

                m_renderer = std::make_unique<vulkan_renderer>();
                if (!m_renderer->create())
                {
                        IC_CORE_ERROR("Failed to initialize renderer!");
                        return false;
                }

                m_initialized = true;
                IC_CORE_INFO("Renderer Initialized");
                return true;
        }

        void renderer::renderFrame()
        {
                IC_CORE_FATAL_IF(!m_initialized,
                                 "Render called before Initialization!"); // TODO: fix that if renderer not initialized
                                                                          // it never comes here
                m_renderer->drawFrame();
                // IC_INFO("Renderer Called!");
        }

        void renderer::cleanUp()
        {
                if (!m_initialized)
                        return;
                m_initialized = false;
                m_renderer->destroy(m_context.get()->getDevice()->get());
                m_context->cleanUp();
                IC_CORE_INFO("Renderer Cleaned Up!");
        }

} // namespace ic
