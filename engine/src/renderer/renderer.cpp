#include "renderer.h"

namespace ic
{

        bool renderer::init(window* w)
        {
                if (m_initialized)
                {
                        IC_CORE_WARN("Renderer already initialized");
                        return false;
                }

                m_context = std::make_unique<vulkan_context>(*w);

                if (!m_context->initialize(true))
                {
                        IC_CORE_ERROR("Failed to initialize context!");
                        return false;
                }

                m_renderer = std::make_unique<vulkan_renderer>(m_context.get(), m_context->getVulkanDevice());
                if (!m_renderer->init())
                {
                        IC_CORE_ERROR("Failed to initialize renderer!");
                        return false;
                }

                m_initialized = true;
                IC_CORE_INFO("Renderer Initialized");
                return true;
        }

        void renderer::onEvent(event& e)
        {
                m_renderer->onEvent(e);
        }

        void renderer::renderFrame(float deltaTime)
        {
                IC_CORE_FATAL_IF(!m_initialized,
                                 "Render called before Initialization!");  // TODO: fix that if renderer not initialized
                                                                           // it never comes here
                m_renderer->render(deltaTime);
                // IC_INFO("Renderer Called!");
        }

        void renderer::cleanUp()
        {
                if (!m_initialized)
                        return;
                m_initialized = false;
                m_renderer->destroy();
                m_context->cleanUp();
                IC_CORE_INFO("Renderer Cleaned Up!");
        }

}  // namespace ic
