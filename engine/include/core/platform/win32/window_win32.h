#ifndef WINDOW_WIN32_H
#define WINDOW_WIN32_H

#include "defines.h"

#include "core/window.h"

#include "core/events/event.h"
#include "core/events/application_event.h"
#include "core/events/mouse_event.h"
#include "core/events/key_event.h"

struct GLFWwindow;

namespace ic
{

class win32_window : public Window
{
public:
        win32_window(const window_props &props);
        ~win32_window() override;

        void onUpdate() override;

        unsigned int getWidth() const override { return m_data.width; }
        unsigned int getHeight() const override { return m_data.height; }

        // Window attributes
        void setEventCallback(const eventCallbackFn &callback) override { m_data.eventCallback = callback; }
        void setVSync(bool enabled) override;
        bool isVSync() const override;

        GLFWwindow *GetNativeWindow() const override { return m_Window; }

        bool        wasWindowResized() { return framebufferResized; }
        static void framebufferResizeCallback(GLFWwindow *handle, int width, int height);

        bool framebufferResized = false;

private:
        void init(const window_props &props);
        void shutdown();

private:
        GLFWwindow *m_Window;
        // GraphicsContext* m_Context;

        struct window_data
        {
                const char  *title;
                unsigned int width, height;
                bool         VSync;

                eventCallbackFn eventCallback;
        };

        window_data m_data;
};

}  // namespace ic

#endif