#include "core/application.h"
#include "core/input.h"
#include "core/logger.h"
#include "core/filesystem.h"
#include "core/allocators.h"
#include "core/assets/asset_manager.h"
#include "renderer/renderer.h"

#include <GLFW/glfw3.h>

// TODO: add a linux build with wayland to start building with valgrind memory checks
// TODO: maybe write a memory effecient class for checking how much memory is being used. I suspect that memory of
// Validation layers of vilkan engine is being leaked

namespace ic
{

Application *Application::s_Instance = nullptr;

Application::Application(window_props &properties)
{
        isRunning = true;
        ic::logger::init();

        m_Window = Window::create(properties);
        m_Window->setEventCallback(BIND_EVENT(onEvent));

        fs_init();
        AssetManager::Initialize("assets/registry.yaml");

        // TODO: Not by build system but by UI systems. This makes application reloads so handle that
        m_renderer = createRenderer(RendererAPI::OpenGL);
        m_renderer->init(m_Window.get());

        IC_CORE_INFO("Application Initialized!");
}

Application::~Application()
{
        AssetManager::Shutdown();
        fs_deinit();
        m_renderer->cleanUp();
        destroyRenderer(m_renderer);
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

void Application::onEvent(event &e)
{
        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowClosedEvent>(BIND_EVENT(onWindowClose));

        m_renderer->onEvent(e);
}

Application &Application::get()
{
        return *s_Instance;
}
bool Application::onWindowClose(WindowClosedEvent &e)
{
        isRunning = false;
        return true;
}

}  // namespace ic

void ic_create_application(ic::window_props *windowProperties)
{
        if (ic::Application::s_Instance)
                return;
        void *application_memory    = ic_malloc(sizeof(ic::Application));
        ic::Application::s_Instance = new (application_memory) ic::Application(*windowProperties);
}

bool ic_app_is_running(void)
{
        return ic::Application::get().isRunning;
}

void ic_app_set_callback(AppUpdateFn update_fn, AppRenderFn render_fn)
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
        ic::Application::get().~Application();
        ic_free(ic::Application::s_Instance);

#if defined(_DEBUG)
        ic::heap_dump_leaks();
#endif
        ic::Application::s_Instance = nullptr;
}
