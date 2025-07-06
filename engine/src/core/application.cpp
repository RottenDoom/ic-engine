#include "application.h"
#include "defines.h"

// TODO: Refactor the code to be more modular and easier to understand
// TODO: Add logging wherever required
// TODO: Add input system so that it actually works.
// TODO: Create input system and start creating the graphics library frontend first and then the backend.

namespace ic {
#define BIND_EVENT_FN(x) std::bind(&application::x, this, std::placeholders::_1)

    application* application::s_Instance = nullptr;

    application::application()
    {
        s_Instance = this;

        m_Window = window::create();
        m_Window->setEventCallback(BIND_EVENT_FN(onEvent));

        // Initialize the logger testing logger
        ic::logger::init();
    }

    application::~application()
    {
        delete m_Window;
    }

    bool application::run()
    {
        while (m_Running)
        {
            m_Window->onUpdate();
        }

        return true;
    }

    void application::onEvent(event & e)
    {
        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowClosedEvent>(BIND_EVENT_FN(on_window_close));

        IC_CORE_TRACE("{0}", e.getName());
    }

    bool application::application_create(game* game_inst)
    {
        return true;
    }

    application& application::Get()
    {
        return *s_Instance;
    }
    bool application::on_window_close(WindowClosedEvent & e)
    {
        m_Running = false;
        return true;
    }
}