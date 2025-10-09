#pragma once
#include "defines.h"
#include "vulkan/context.h"
#include "vulkan/vulkan_renderer.h"

/*
 * The renderer class is nothing but a cross platformer frontend for my engine. I am writing this engine currently in
 * vulkan. My engine in probably also contain support for other graphic API (hopefully) and I would like this class to
 * be the main hub for all three to present their code here.
 */

namespace ic
{
        class renderer
        {
        private:
                bool m_initialized = false;
                std::unique_ptr<vulkan_context> m_context;
                std::unique_ptr<vulkan_renderer> m_renderer;

        public:
                renderer()          = default;
                virtual ~renderer() = default;

                bool init(GLFWwindow* window);
                void renderFrame();
                void cleanUp();
        };
}  // namespace ic