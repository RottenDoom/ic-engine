#include "event.h"

event::event() {
    is_initialized = FALSE;
}

event::~event() {
}

b8 event::event_initialize() {
    if (is_initialized == TRUE) {
        return FALSE;
    }
    is_initialized = FALSE;
    mem.ic_zero_memory(&m_state, sizeof(m_state));

    is_initialized = TRUE;

    return TRUE;
}

void event::event_shutdown() {
    // Free the events arrays. And objects pointed to should be destroyed on their own.
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(m_state.registered[i].events != 0) {
            arr.darray_destroy(m_state.registered[i].events);
            m_state.registered[i].events = 0;
        }
    }
}

b8 event::event_register(u16 code, void* listener, PFN_on_event on_event) {
    if(is_initialized == FALSE) {
        return FALSE;
    }

    if(m_state.registered[code].events == 0) {
        m_state.registered[code].events = (registered_event*)darray_type_create(registered_event);
    }

    u64 registered_count = arr.darray_length(m_state.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        if(m_state.registered[code].events[i].m_listener == listener) {
            // TODO: warn about more than once same listener
            return FALSE;
        }
    }

    // If at this point, no duplicate was found. Proceed with registration.
    registered_event event;
    event.m_listener = listener;
    event.callback = on_event;
    arr.darray_push(m_state.registered[code].events, &event);

    return TRUE;
}

b8 event::event_unregister(u16 code, void* listener, PFN_on_event on_event) {
    if(is_initialized == FALSE) {
        return FALSE;
    }

    // On nothing is registered for the code, boot out.
    if(m_state.registered[code].events == 0) {
        // TODO: warn
        return FALSE;
    }

    u64 registered_count = arr.darray_length(m_state.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        registered_event e = m_state.registered[code].events[i];
        if(e.m_listener == listener && e.callback == on_event) {
            // Found one, remove it
            registered_event popped_event;
            arr.darray_pop_at(m_state.registered[code].events, i, &popped_event);
            return TRUE;
        }
    }

    // Not found.
    return FALSE;
}


b8 event::event_fire(u16 code, void* sender, event_context context) {
    if(is_initialized == FALSE) {
        return FALSE;
    }

    // If nothing is registered for the code, boot out.
    if(m_state.registered[code].events == 0) {
        return FALSE;
    }

    u64 registered_count = arr.darray_length(m_state.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        registered_event e = m_state.registered[code].events[i];
        if(e.callback(code, sender, e.m_listener, context)) {
            // Message has been handled, do not send to other listeners.
            return TRUE;
        }
    }

    // Not found.
    return FALSE;
}
