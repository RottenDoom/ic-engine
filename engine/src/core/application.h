#pragma once
#include "events/application_event.h"
#include "events/event.h"
#include "renderer/renderer.h"
#include "window.h"

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
                static GLFWwindow* getWindow() { return (GLFWwindow*)s_Instance->m_Window->getNativeWindow(); }

        private:
                bool onWindowClose(WindowClosedEvent& e);

                bool m_Running = true;
                window* m_Window;
                renderer* m_renderer;
                static application* s_Instance;
                float m_lastFrameTime = 0.0f;
        };
}  // namespace ic