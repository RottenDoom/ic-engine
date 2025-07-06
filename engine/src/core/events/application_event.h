#pragma once

#include "event.h"

namespace ic
{
    class WindowResizedEvent : public event
    {
    public:
        WindowResizedEvent(unsigned int width, unsigned int height)
            : m_width(width), m_height(height) {}

        inline unsigned int getWidth() const { return m_width; }
        inline unsigned int getHeight() const { return m_height; }

        const char* toString() const override
        {
            std::stringstream ss;
            ss << "WindowResizedEvent: " << m_width << ", " << m_height;
            return ss.str().c_str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        unsigned int m_width, m_height;
    };

    class WindowClosedEvent : public event
    {
    public:
        WindowClosedEvent() {}

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppTickEvent : public event
    {
    public:
        AppTickEvent() {}

        EVENT_CLASS_TYPE(AppTick)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppUpdateEvent : public event
    {
    public:
        AppUpdateEvent() {}

        EVENT_CLASS_TYPE(AppUpdate)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
} // namespace ic