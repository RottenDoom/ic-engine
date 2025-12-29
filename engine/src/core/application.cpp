#include "core/application.h"
#include "core/input.h"
#include "core/logger.h"

#include <GLFW/glfw3.h>

// TODO: add a linux build with wayland to start building with valgrind memory checks
// TODO: maybe write a memory effecient class for checking how much memory is being used. I suspect that memory of
// Validation layers of vilkan engine is being leaked

namespace ic
{

Application* Application::s_Instance = nullptr;

Application::Application(window_props& properties)
{
        isRunning = true;
        ic::logger::init();

        m_Window = Window::create(properties);
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
        while (isRunning)
        {
                float time      = glfwGetTime();
                float delta     = time - m_lastFrameTime;
                m_lastFrameTime = time;

                if (user_update)
                {
                        user_update(delta);
                }

                m_Window->onUpdate();

                if (user_render)
                {
                        user_render();
                }
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

Application& Application::get()
{
        return *s_Instance;
}
bool Application::onWindowClose(WindowClosedEvent& e)
{
        isRunning = false;
        return true;
}

}  // namespace ic

void ic_create_application(ic::window_props* windowProperties)
{
        if (ic::Application::s_Instance)
                return;

        ic::Application::s_Instance = new ic::Application(*windowProperties);
}

bool ic_app_is_running(void)
{
        return ic::Application::get().isRunning;
}

void ic_app_set_callback(ic::AppUpdateFn update_fn, ic::AppRenderFn render_fn)
{
        ic::Application::get().user_update = update_fn;
        ic::Application::get().user_render = render_fn;
}

void ic_app_run(void)
{
        ic::Application::get().run();
}

void ic_app_destroy(void)
{
        delete ic::Application::s_Instance;
        ic::Application::s_Instance = nullptr;
}
