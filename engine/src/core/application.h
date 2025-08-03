#pragma once
#include "window.h"
#include "events/event.h"
#include "events/application_event.h"
#include "renderer/renderer.h"

class game;

namespace ic
{
    class application
    {
    public:
        application();
        ~application();

        bool run();
        void onEvent(event& e);
        bool applicationCreate(game* game_inst);

        static application& get();
        inline window& getWindow() { return *m_Window; }

    private:
        bool onWindowClose(WindowClosedEvent& e);
        // bool onWindowResize(WindowResizedEvent& e);

        bool m_Running = true;
        window* m_Window;
        renderer* m_renderer;
        static application* s_Instance;
    };
} // namespace ic