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
        m_Window->setEventCallback(BIND_EVENT(OnEvent));

        s_Instance = this;
}

void Application::Initialize()
{
        fs_init();
        AssetManager::Initialize("assets/registry.yaml");

        // TODO: Not by build system but by UI systems. This makes application reloads so handle that
        m_renderer = create_renderer(RendererAPI::OpenGL);
        m_renderer->Init(m_Window);

        IC_CORE_INFO("Application Initialized!");
}

Application::~Application()
{
        AssetManager::Shutdown();
        fs_deinit();
        m_renderer->CleanUp();
        destroy_renderer(m_renderer);
        delete m_Window;
}

bool Application::Run()
{
	/** TODO: Replace glfw dependencies with my own. */
        while (isRunning)
        {
                float time      = glfwGetTime();
                float delta     = time - m_lastFrameTime;
                m_lastFrameTime = time;

		glfwPollEvents();

		// user update
                if (user_update)
                {
                        user_update(delta);
                }

		// engine render
		m_renderer->RenderFrame(delta);

		// UI + user render
                if (user_render)
                {
                        user_render();
                }
		glfwSwapBuffers(m_Window->GetNativeWindow());
        }

        return true;
}

void Application::OnEvent(event &e)
{
        eventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowClosedEvent>(BIND_EVENT(OnWindowClose));

        m_renderer->OnEvent(e);
}

// TODO rewrite this function
bool Application::OnWindowClose(WindowClosedEvent &e)
{
        isRunning = false;
        return true;
}

void Application::SetFnPointers(AppUpdateFn update_fn, AppRenderFn render_fn)
{
        user_update = update_fn;
        user_render = render_fn;
}

}  // namespace ic

static ic::Application *s_app = nullptr;

void ic_clear_color(void)
{
	ic::Application::Get().GetRenderer()->ClearColor();
}

void ic_clear_buffer_bit(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void ic_create_application(ic::window_props *windowProperties)
{
        s_app = new ic::Application(*windowProperties);
        s_app->Initialize();
}

bool ic_app_is_running(void)
{
        return ic::Application::Get().IsAppRunning();
}

void ic_app_set_callback(AppUpdateFn update_fn, AppRenderFn render_fn)
{
        ic::Application::Get().SetFnPointers(update_fn, render_fn);
}

void ic_app_run(void)
{
        ic::Application::Get().Run();
}

void ic_app_destroy(void)
{
        delete s_app;
        s_app = nullptr;
#ifndef NDEBUG
        ic::heap_dump_leaks();
#endif
}
