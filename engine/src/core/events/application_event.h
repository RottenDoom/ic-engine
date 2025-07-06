#pragma once

#include "event.h"

namespace ic
{
    class WindowResizedEvent : public event
    {
    public:
        WindowResizedEvent(uint32_t width, uint32_t height)
            : m_width(width), m_height(height) {}

        inline uint32_t getWidth() const { return m_width; }
        inline uint32_t getHeight() const { return m_height; }

        std::string toString() const override
        {
            std::stringstream ss;
            ss << "WindowResizedEvent: " << m_width << ", " << m_height;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        uint32_t m_width, m_height;
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