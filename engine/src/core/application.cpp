#include "application.h"
#include "defines.h"
#include "input.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// TODO: add a linux build with wayland to start building with valgrind memory checks
// TODO: maybe write a memory effecient class for checking how much memory is being used. I suspect that memory of
// Validation layers of vilkan engine is being leaked

namespace ic
{
#define BIND_EVENT_FN(x) std::bind(&application::x, this, std::placeholders::_1)

        application* application::s_Instance = nullptr;

        application::application()
        {
                ic::logger::init();
                s_Instance = this;

                m_Window   = Window::create();
                m_Window->setEventCallback(BIND_EVENT_FN(onEvent));

                m_renderer = new renderer();
                m_renderer->init(m_Window.get());
                IC_CORE_INFO("Application Initialized!");
        }

        application::~application()
        {
                m_renderer->cleanUp();
                delete m_renderer;
        }

        bool application::run()
        {
                while (m_Running)
                {
                        float time      = glfwGetTime();
                        float delta     = time - m_lastFrameTime;
                        m_lastFrameTime = time;

                        m_Window->onUpdate();
                        m_renderer->renderFrame(delta);
                }

                return true;
        }

        void application::onEvent(event& e)
        {
                eventDispatcher dispatcher(e);
                dispatcher.dispatch<WindowClosedEvent>(BIND_EVENT_FN(onWindowClose));

                m_renderer->onEvent(e);

                // IC_CORE_TRACE("{0}", e.toString()); TODO: get a better understanding of this
        }

        bool application::applicationCreate(game* game_inst)
        {
                return true;
        }

        application& application::get()
        {
                return *s_Instance;
        }
        bool application::onWindowClose(WindowClosedEvent& e)
        {
                m_Running = false;
                return true;
        }
}  // namespace ic