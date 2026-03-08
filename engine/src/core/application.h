#pragma once
#include "ic_api.h"
#include "events/application_event.h"
#include "events/event.h"
#include "renderer/renderer.h"
#include "window.h"

class game;

namespace ic
{
        class IC_API application
        {
        public:
                application();
                ~application();

                bool run();
                void onEvent(event& e);
                bool applicationCreate(game* game_inst);

                static application& get();
                Window& getWindow() { return *m_Window; }

        private:
                bool onWindowClose(WindowClosedEvent& e);

                bool m_Running = true;
                std::unique_ptr<Window> m_Window;
                renderer* m_renderer;
                static application* s_Instance;
                float m_lastFrameTime = 0.0f;
        };
}  // namespace ic