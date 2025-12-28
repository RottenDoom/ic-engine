#pragma once
#include "../ic_api.h"
#include "events/application_event.h"
#include "events/event.h"
#include "../renderer/renderer.h"
#include "window.h"

class game;

namespace ic
{
class IC_API Application
{
public:
        Application();
        virtual ~Application();

        bool run();
        void onEvent(event& e);
        bool applicationCreate(game* game_inst);

        static Application& get();
        Window& getWindow() { return *m_Window; }

private:
        bool onWindowClose(WindowClosedEvent& e);

        game* m_game;  // the game instance

        bool m_Running = true;
        std::unique_ptr<Window> m_Window;
        renderer* m_renderer;
        static Application* s_Instance;
        float m_lastFrameTime = 0.0f;
};
}  // namespace ic