#pragma once
#include "../defines.h"

/*
 * The renderer class is nothing but a cross platformer frontend for my engine. I am writing this engine currently in
 * vulkan. My engine in probably also contain support for other graphic API (hopefully) and I would like this class to
 * be the main hub for all three to present their code here.
 */

namespace ic
{
class Window;
class event;
class renderer
{
private:
        struct backend_context;
        struct backend_renderer;

        bool m_initialized = false;
        std::unique_ptr<backend_context> m_context;
        std::unique_ptr<backend_renderer> m_renderer;

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