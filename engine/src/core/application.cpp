#include "core/application.h"
#include "defines.h"
#include "core/input.h"
#include "entry.h"

#include <GLFW/glfw3.h>

// TODO: add a linux build with wayland to start building with valgrind memory checks
// TODO: maybe write a memory effecient class for checking how much memory is being used. I suspect that memory of
// Validation layers of vilkan engine is being leaked

namespace ic
{

Application* Application::s_Instance = nullptr;

Application::Application()
{
        ic::logger::init();
        s_Instance = this;

        m_Window   = Window::create();
        m_Window->setEventCallback(BIND_EVENT(onEvent));

        m_renderer = new renderer();
        m_renderer->init(m_Window.get());
        IC_CORE_INFO("Application Initialized!");
}

Application::~Application()
{
        m_renderer->cleanUp();
        delete m_renderer;
}

bool Application::run()
{
        while (m_Running)
        {
                float time      = glfwGetTime();
                float delta     = time - m_lastFrameTime;
                m_lastFrameTime = time;

                /** TODO: not sure if it works like this  */
                m_game->update(m_game, delta);
                m_game->render(m_game, delta);

                m_Window->onUpdate();
                m_renderer->renderFrame(delta);
        }

        return true;
}

void Application::onEvent(event& e)
{
        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowClosedEvent>(BIND_EVENT(onWindowClose));

        m_renderer->onEvent(e);
}

bool Application::applicationCreate(game* game_inst)
{
        m_game = game_inst;

        if (!m_game->initialize(m_game))
        {
                return false;
        }
        return true;
}

Application& Application::get()
{
        return *s_Instance;
}
bool Application::onWindowClose(WindowClosedEvent& e)
{
        m_Running = false;
        return true;
}
}  // namespace ic