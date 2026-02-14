#ifndef EVENT_H
#define EVENT_H
#include "../../defines.h"
/** TODO: write this class so user can write his own events. */

#define BIT(x) (1 << x)

namespace ic
{
enum class EventType
{
        None = 0,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        AppTick,
        AppUpdate,
        AppRender,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled
};

enum EventCategory
{
        None                     = 0,
        EventCategoryApplication = BIT(0),
        EventCategoryInput       = BIT(1),
        EventCategoryKeyBoard    = BIT(2),
        EventCategoryMouse       = BIT(3),
        EventCategoryMouseButton = BIT(4),
};

// macros to check class types and category
#define EVENT_CLASS_TYPE(type)                                                                                         \
        static EventType getStaticType()                                                                               \
        {                                                                                                              \
                return EventType::type;                                                                                \
        }                                                                                                              \
        virtual EventType getEventType() const override                                                                \
        {                                                                                                              \
                return getStaticType();                                                                                \
        }                                                                                                              \
        virtual const char *getName() const override                                                                   \
        {                                                                                                              \
                return #type;                                                                                          \
        }

#define EVENT_CLASS_CATEGORY(category)                                                                                 \
        virtual int getCategoryFlags() const override                                                                  \
        {                                                                                                              \
                return category;                                                                                       \
        }

class event
{
        friend class eventDispatcher;

public:
        virtual ~event()                       = default;

        bool handled                           = false;

        virtual EventType getEventType() const = 0;
        virtual const char *getName() const    = 0;
        virtual int getCategoryFlags() const   = 0;
        virtual std::string toString() const { return getName(); }

        inline bool isIncharge(EventCategory category) { return getCategoryFlags() & category; }
};

class eventDispatcher
{
public:
        eventDispatcher(event &event) : m_event(event) {}

        template <typename T, typename F>
        bool dispatch(const F &func)
        {
                if (m_event.getEventType() == T::getStaticType())
                {
                        m_event.handled |= func(static_cast<T &>(m_event));
                        return true;
                }
                return false;
        }

private:
        event &m_event;
};

inline std::ostream &operator<<(std::ostream &os, const event &e)
{
        return os << e.toString();
}

}  // namespace ic

#endif