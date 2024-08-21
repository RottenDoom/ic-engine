#pragma once

#include "defines.h"
#include <string>

#define BIT(x) (1 << x)

// enum class is basically an enum of classes
enum class event_type
{
    None = 0,
    window_close, window_resize, window_focus, window_lost_focus, window_moved,
    app_tick, app_update, app_render,
    key_pressed, key_released, key_typed,
    mouse_button_pressed, mouse_button_released, mouse_moved, mouse_scrolled
};

enum event_category
{
    None = 0,
    event_category_application = BIT(0),
    event_category_input = BIT(1),
    event_category_keyboard = BIT(2),
    event_category_mouse = BIT(3),
    event_category_mouse_button = BIT(4)
};

#define EVENT_CLASS_TYPE(type) static event_type get_static_type() { return event_type::type; }\
							   virtual event_type get_event_type() const override { return get_static_type(); }\
							   virtual const char* get_name() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int get_category_flags() const override { return category; }

class IC_API event {
    friend class event_dispatcher;

public:
    virtual ~event() = default;

    bool handled = FALSE;

    virtual event_type get_event_type() const = 0;
    virtual const char* get_name() const = 0;
    virtual int get_category_flags() const = 0;
    virtual std::string to_string() const { return get_name(); }

    inline bool is_incharge(event_category category)
    {
        return get_category_flags() & category;
    }
};

class event_dispatcher {
    template<typename T>
    using event_fn = std::function<bool(T&)>; // function pointer using function from std lib instead of using PFN_on_event

public:
    event_dispatcher(event& e)
        : m_event(e)
    {}

    template<typename T>
    bool dispatch(event_fn<T> func)
    {
        if (m_event.get_event_type() == T::get_static_type())
        {
            m_event.handled |= func(*(T*)& m_event);
            return TRUE;
        }
        return FALSE;
    }

private:
    event& m_event;
};

inline std::ostream& operator<<(std::ostream& os, const event& e)
{
    return os << e.to_string();
}