#include "application.h"
#include "defines.h"
#include "input.h"

// TODO: Refactor code (refactor device.cpp and make files )
// TODO: renderer initiailization done. renderer device creation and queues left
// TODO: start actually rendering.
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

                m_Window   = window::create();
                m_Window->setEventCallback(BIND_EVENT_FN(onEvent));

                m_renderer = new renderer();
                m_renderer->init(getWindow());
                IC_CORE_INFO("Application Initialized!");
        }

        application::~application()
        {
                m_renderer->cleanUp();
                delete m_renderer;
                delete m_Window;
        }

        bool application::run()
        {
                while (m_Running)
                {
                        m_Window->onUpdate();
                        m_renderer->renderFrame();

                        // update and delta time here.
                }

                return true;
        }

        void application::onEvent(event& e)
        {
                eventDispatcher dispatcher(e);
                dispatcher.dispatch<WindowClosedEvent>(BIND_EVENT_FN(onWindowClose));

                IC_CORE_TRACE("{0}", e.toString());
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
        bool application::onWindowResize(WindowResizedEvent& e)
        {
                /// TODO: look into the resizing part
                while (m_Window->getHeight() == 0 || m_Window->getWidth() == 0)
                {
                        glfwWaitEvents();
                }
                return true;
        }
}  // namespace ic