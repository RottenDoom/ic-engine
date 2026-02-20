#ifndef APPLICATION_H
#define APPLICATION_H

#include "defines.h"
#include "window.h"

#include "events/application_event.h"
#include "events/event.h"

namespace ic
{

class IRenderer;

class IC_API Application
{
public:
        bool isRunning           = true;
        AppUpdateFn *user_update = nullptr;
        AppRenderFn *user_render = nullptr;

        static Application *s_Instance;

public:
        Application(window_props &properties);
        virtual ~Application();

        bool run();
        void onEvent(event &e);

        static Application &get();
        Window &getWindow() { return *m_Window; }
        IRenderer *getRenderer() { return m_renderer; }

private:
        bool onWindowClose(WindowClosedEvent &e);

        std::unique_ptr<Window> m_Window;
        IRenderer *m_renderer;
        float m_lastFrameTime = 0.0f;
};

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif
        /**
         * @function ic_create_application
         * @category app
         * @brief Creates the ic_engine application
         * @param ic::window_props takes in a string title, width and height
         * @related ic_app_destroy ic_app_set_callback
         */
        IC_API void ic_create_application(ic::window_props *windowProperties);

        /**
         * @function ic_app_is_running
         * @category app
         * @brief Checks if the application is running
         * @return Returns true if application is running
         */
        IC_API bool ic_app_is_running(void);

        /**
         * @function ic_app_set_callback
         * @category app
         * @brief Sets the update and render callbacks for the user
         * @param AppUpdateFn: user update function that takes form void update(float deltatime);
         * @param AppRenderFn: user render function that taken form void update(void);
         * @related ic_app_run
         */
        IC_API void ic_app_set_callback(AppUpdateFn update_fn, AppRenderFn render_fn);

        /**
         * @function is_app_run
         * @category app
         * @brief Run the application
         * @related ic_create_application
         */
        IC_API void ic_app_run(void);

        /**
         * @function
         * @category app
         * @brief Destroys the application and cleans up any resources
         * @related ic_create_application
         */
        IC_API void ic_app_destroy(void);

#ifdef __cplusplus
}
#endif

#endif