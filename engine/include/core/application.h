#pragma once

#include "../defines.h"
#include "events/application_event.h"
#include "events/event.h"
#include "../renderer/renderer.h"
#include "window.h"

namespace ic
{

/* -------- User-defined callbacks -------- */
/** TODO: add this in defines */
typedef void(AppUpdateFn)(float dt);
typedef void(AppRenderFn)(void);
class IC_API Application
{
public:
        bool isRunning           = true;
        AppUpdateFn* user_update = nullptr;
        AppRenderFn* user_render = nullptr;

        static Application* s_Instance;

public:
        Application(window_props& properties);
        virtual ~Application();

        bool run();
        void onEvent(event& e);

        static Application& get();
        Window& getWindow() { return *m_Window; }

private:
        bool onWindowClose(WindowClosedEvent& e);

        std::unique_ptr<Window> m_Window;
        renderer* m_renderer;
        float m_lastFrameTime = 0.0f;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif

        // Start making the header files while also including header gauds on top
        // This header file will contain the API to load an application. I want to load the application using a dll so
        // we will learn that as well to learn that casey mutori is the god

        IC_API void ic_create_application(ic::window_props* windowProperties);

        IC_API bool ic_app_is_running(void);
        IC_API void ic_app_set_callback(ic::AppUpdateFn update_fn, ic::AppRenderFn render_fn);
        IC_API void ic_app_run(void);

        IC_API void ic_app_destroy(void);

#ifdef __cplusplus
}
#endif
