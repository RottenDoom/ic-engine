#pragma once

#include "defines.h"

#include "event.h"
#include <sstream>

// application events
class window_resized_event : public event
{
public:
    window_resized_event(u32 width, u32 height) 
        : m_width(width), m_height(height) {}
    
    inline u32 get_width() const { return m_width; }
    inline u32 get_height() const { return m_height; }

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "window_resized_event:" << m_width << ", " << m_height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(window_resize)
    EVENT_CLASS_CATEGORY(event_category_application)
private:
    u32 m_width, m_height;
};

class window_close_event : public event
{
public:
    window_close_event() {}

    EVENT_CLASS_TYPE(window_close)
    EVENT_CLASS_CATEGORY(event_category_application)
};

class app_tick_event : public event
{
public:
    app_tick_event() {}

    EVENT_CLASS_TYPE(app_tick)
    EVENT_CLASS_CATEGORY(event_category_application)
};

class app_update_event : public event
{
public:
    app_update_event() {}

    EVENT_CLASS_TYPE(app_update)
    EVENT_CLASS_CATEGORY(event_category_application)
};

// keyboard events
class key_event : public event
{
public:
    inline i32 get_key_code() const { return m_key_code; }

    EVENT_CLASS_CATEGORY(event_category_keyboard | event_category_input)

protected:
    key_event(int keycode)
        : m_key_code(keycode) {}

    i32 m_key_code;
};

class key_pressed_event : public key_event
{
public:
    key_pressed_event(i32 keycode, i32 repeat_count)
        : key_event(keycode), m_repeat_count(repeat_count) {}

    inline u32 get_repeat_count() const { return m_repeat_count; }

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "key_pressed_event: " << m_key_code << " (" << m_repeat_count << " repeats)";
        return ss.str();
    }

    EVENT_CLASS_TYPE(key_pressed)
    
private:
    u32 m_repeat_count;
};

class key_released_event : public key_event
{
    key_released_event(u32 keycode)
        : key_event(keycode) {}

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "key_released: " << m_key_code;
        return ss.str();
    }

    EVENT_CLASS_TYPE(key_released)
private:
    u32 m_key_code;
};

class key_typed_event : public key_event
	{
	public:
		key_typed_event(u32 keycode)
			: key_event(keycode) {}

		std::string to_string() const override
		{
			std::stringstream ss;
			ss << "key_typed_event: " << m_key_code;
			return ss.str();
		}
		
		EVENT_CLASS_TYPE(key_typed)
private:
    u32 m_key_code;
};

// mouse events
class mouse_moved_event : public event
{
public:
    mouse_moved_event(f32 x, f32 y)
        : m_mouse_x(x), m_mouse_y(y) {}

    inline float get_x() const { return m_mouse_x; }
    inline float get_y() const { return m_mouse_y; }

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "mouse_moved_event: " << m_mouse_x << ", " << m_mouse_y;
        return ss.str();
    }

    EVENT_CLASS_TYPE(mouse_moved)
    EVENT_CLASS_CATEGORY(event_category_mouse | event_category_input)

private:
    f32 m_mouse_x;
    f32 m_mouse_y;
};

class mouse_scrolled_event : public event
{
public:
    mouse_scrolled_event(f32 x_offset, f32 y_offset)
        : m_x_offset(x_offset), m_y_offset(y_offset) {}

    inline f32 get_x_offset() const { return m_x_offset; }
    inline f32 get_y_offset() const { return m_y_offset; }

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "mouse_scrolled_event: " << m_x_offset << ", " << m_y_offset;
        return ss.str();
    }
private:
    f32 m_x_offset;
    f32 m_y_offset;
};

class mouse_button_event : public event
{
public:
    inline u32 get_mouse_button() const { return m_button; }

    EVENT_CLASS_CATEGORY(event_category_mouse | event_category_input)
protected:
    mouse_button_event(u32 button)
        : m_button(button) {}

    u32 m_button;
};

class mouse_button_pressed_event : public mouse_button_event
{
public:
    mouse_button_pressed_event(u32 button)
        : mouse_button_event(button) {}
    
    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "mouse_button_pressed_event: " << m_button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(mouse_button_pressed)
};

class mouse_button_released_event : public mouse_button_event
{
public:
    mouse_button_released_event(u32 button)
        : mouse_button_event(button) {}

    std::string to_string() const override
    {
        std::stringstream ss;
        ss << "mouse_released_pressed_event: " << m_button;
        return ss.str();
    }

    EVENT_CLASS_TYPE(mouse_button_released)
};