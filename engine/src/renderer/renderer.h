#pragma once
#include "defines.h"

/*
 * The renderer class is nothing but a cross platformer frontend for my engine. I am writing this engine currently in
 * vulkan. My engine in probably also contain support for other graphic API (hopefully) and I would like this class to
 * be the main hub for all three to present their code here.
 */

namespace ic
{
        class vulkan_context;
        class vulkan_renderer;
        class Window;
        class event;
        class renderer
        {
        private:
                bool m_initialized = false;
                std::unique_ptr<vulkan_context> m_context;
                std::unique_ptr<vulkan_renderer> m_renderer;

        public:
                explicit renderer();
                ~renderer();

                renderer(const renderer&)            = delete;
                renderer& operator=(const renderer&) = delete;

                bool init(Window* w);
                void onEvent(event& e);
                void renderFrame(float deltaTime);
                void cleanUp();
        };
}  // namespace ic