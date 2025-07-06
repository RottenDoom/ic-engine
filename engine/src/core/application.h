#pragma once
#include "window.h"
#include "events/event.h"
#include "events/application_event.h"

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
        bool application_create(game* game_inst);

        static application& Get();
        inline window& GetWindow() { return *m_Window; }

    private:
        bool on_window_close(WindowClosedEvent& e);
        // bool on_window_resize(WindowResizedEvent& e);

        bool m_Running = true;
        window* m_Window;
        static application* s_Instance;
    };
}