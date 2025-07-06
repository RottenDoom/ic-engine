#include "application.h"
#include "defines.h"
#include "input.h"

// TODO: Refactor the code to be more modular and easier to understand
// TODO: Add logging wherever required
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
    bool application::onWindowClose(WindowClosedEvent & e)
    {
        m_Running = false;
        return true;
    }
} // namespace ic