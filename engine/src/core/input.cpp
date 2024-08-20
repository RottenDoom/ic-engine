#include "input.h"

input::input() {
    is_initialized = FALSE;
}

input::~input() {
}


void input::input_process_button(buttons button, b8 pressed) {
    // if state changes, fire event
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;

        // Fire event
        event_context context;
        context.data.u16[0] = button;
        m_event.event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input::input_process_mouse_move(i16 x, i16 y) {
    // Only process if actually different
    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        // NOTE: Enable this if debugging.
        //IC_DEBUG("Mouse pos: %i, %i!", x, y);

        // Update internal state.
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        // Fire the event.
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        m_event.event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input::input_process_mouse_wheel(i8 z_delta) {

    // Fire the event.
    event_context context;
    context.data.u8[0] = z_delta;
    m_event.event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

void input::input_process_key(keys key, b8 pressed) {
    // Only handle this if the state actually changed.
    if (state.keyboard_current.keys[key] != pressed) {
        // Update internal state.
        state.keyboard_current.keys[key] = pressed;

        // Fire off an event for immediate processing.
        event_context context;
        context.data.u16[0] = key;
        m_event.event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input::input_initialoize() {
    mem.ic_zero_memory(&state, sizeof(input_state));
    is_initialized = TRUE;
    IC_INFO("Input system initialized.");
}

void input::input_shutdown() {
    is_initialized = FALSE;
}

void input::input_update(f64 delta_time) {
    if (!is_initialized) {
        return;
    }

    // copy current status to previous states
    mem.ic_copy_memory(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    mem.ic_copy_memory(&state.mouse_previous, &state.mouse_current, sizeof(keyboard_state));
}

// keyboard input
b8 input::input_is_key_down(keys key) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.keyboard_current.keys[key] == TRUE;
}

b8 input::input_is_key_up(keys key) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.keyboard_previous.keys[key] == TRUE;
}

b8 input::input_was_key_down(keys key) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.keyboard_previous.keys[key] == TRUE;
}

b8 input::input_was_key_up(keys key) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.keyboard_previous.keys[key] == FALSE;
}

// mouse input
b8 input::input_is_button_down(buttons button) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.mouse_current.buttons[button] == TRUE;
}

b8 input::input_is_button_up(buttons button) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.mouse_current.buttons[button] == FALSE;
}

b8 input::input_was_button_down(buttons button) {
    if (!is_initialized) {
        return FALSE;
    }
    return state.mouse_previous.buttons[button] == TRUE;
}

b8 input::input_was_button_up(buttons button) {
    if (!is_initialized) {
        return TRUE;
    }
    return state.mouse_previous.buttons[button] == FALSE;
}

void input::input_get_mouse_position(i32* x, i32* y) {
    if (!is_initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input::input_get_previous_mouse_position(i32* x, i32* y) {
    if (!is_initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}

