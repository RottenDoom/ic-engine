#include "event.h"

#include "ic_memory.h"
#include "containers/darray.h"
#include "core/logger.h"

static memory mem;
static darray arr;

static b8 is_initialized = FALSE;
static event_system_state state;

b8 event_initialize() {
    mem.ic_zero_memory(&state, sizeof(state));

    is_initialized = TRUE;

    return TRUE;
}

void event_shutdown() {
    // Free the events arrays. And objects pointed to should be destroyed on their own.
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(state.registered[i].events != 0) {
            arr.darray_destroy(state.registered[i].events);
            state.registered[i].events = 0;
        }
    }
}

b8 event_register(u16 code, void* listener, PFN_on_event on_event) {
    if(is_initialized == FALSE) {
        return FALSE;
    }

    if(!state.registered[code].events) {
        state.registered[code].events = (registered_event*)darray_type_create(registered_event);
    }

    u64 registered_count = arr.darray_length(state.registered[code].events); // 0 initially
    // IC_DEBUG("%i", registered_count);
    for(u64 i = 0; i < registered_count; ++i) {
        if(state.registered[code].events[i].m_listener == listener) {
            // TODO: warn about more than once same listener
            return FALSE;
        }
    }

    // If at this point, no duplicate was found. Proceed with registration.
    registered_event event;
    event.m_listener = listener;
    event.callback = on_event;
    arr.darray_push(state.registered[code].events, &event); // increase the length and registers is events array

    return TRUE;
}

b8 event_unregister(u16 code, void* listener, PFN_on_event on_event) {
    if(is_initialized == FALSE) {
        return FALSE;
    }

    // On nothing is registered for the code, boot out.
    if(state.registered[code].events == 0) {
        // TODO: warn
        return FALSE;
    }

    u64 registered_count = arr.darray_length(state.registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        registered_event e = state.registered[code].events[i];
        if(e.m_listener == listener && e.callback == on_event) {
            // Found one, remove it
            registered_event popped_event;
            arr.darray_pop_at(state.registered[code].events, i, &popped_event);
            return TRUE;
        }
    }

    // Not found.
    return FALSE;
}


b8 event_fire(u16 code, void* sender, event_context context) {
    if(is_initialized == FALSE) {
        IC_DEBUG("Events: event not initialized.");
        return FALSE;
    }

    // If nothing is registered for the code, boot out.
    if(!state.registered[code].events) {
        IC_DEBUG("Events: code not registered.");
        return FALSE;
    }

    u64 registered_count = arr.darray_length(state.registered[code].events); // this creating problem
    IC_DEBUG("%i", registered_count);
    for(u64 i = 0; i < registered_count; ++i) {
        registered_event e = state.registered[code].events[i];
        if(e.callback(code, sender, e.m_listener, context)) {
            // Message has been handled, do not send to other listeners.
            return TRUE;
        }
    }
    // Not found.
    return FALSE;
}
