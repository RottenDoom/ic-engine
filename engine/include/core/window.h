#ifndef WINDOW_H
#define WINDOW_H
#include "defines.h"
#include "events/event.h"
namespace ic
{

struct IC_API window_props
{
        const char  *title;
        unsigned int width;
        unsigned int height;

        window_props(const char *title = "IC Engine v0.01", unsigned int width = 1280, unsigned int height = 720)
            : title(title), width(width), height(height)
        {
        }
};

// Interface representing a desktop system based Window
class Window
{
public:
        using eventCallbackFn = std::function<void(event &)>;

        virtual ~Window() = default;

        virtual void onUpdate() = 0;

        virtual unsigned int getWidth() const  = 0;
        virtual unsigned int getHeight() const = 0;

        // Window attributes
        virtual void setEventCallback(const eventCallbackFn &callback) = 0;
        virtual void setVSync(bool enabled)                            = 0;
        virtual bool isVSync() const                                   = 0;

        virtual void *getNativeWindow() const = 0;

        static Window *create(const window_props &props = window_props());
};

}  // namespace ic

#endif