#ifndef KEY_EVENT_H
#define KEY_EVENT_H

#include "../../defines.h"
#include "event.h"
#include "../keycodes.h"

namespace ic
{
class KeyEvent : public event
{
public:
        KeyCode getKeyCode() const { return m_keyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyBoard | EventCategoryInput)
protected:
        KeyEvent(const KeyCode keycode) : m_keyCode(keycode) {}

        KeyCode m_keyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
        KeyPressedEvent(const KeyCode keycode, bool isRepeat = false) : KeyEvent(keycode), m_IsRepeat(isRepeat) {}

        inline uint32_t isRepeat() const { return m_IsRepeat; }

        std::string toString() const override
        {
                std::stringstream ss;
                ss << "KeyPressedEvent: " << m_keyCode << " ( repeat: " << m_IsRepeat << " )";
                return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)

private:
        bool m_IsRepeat;
};

class KeyReleasedEvent : public KeyEvent
{
public:
        KeyReleasedEvent(KeyCode keycode) : KeyEvent(keycode) {}

        std::string toString() const override
        {
                std::stringstream ss;
                ss << "KeyReleasedEvent: " << m_keyCode;
                return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
public:
        KeyTypedEvent(KeyCode keycode) : KeyEvent(keycode) {}

        std::string toString() const override
        {
                std::stringstream ss;
                ss << "KeyTypedEvent: " << m_keyCode;
                return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)

private:
};
}  // namespace ic

#endif
