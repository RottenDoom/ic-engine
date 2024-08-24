#include "application.h"
#include "game_inst.h"

application* application::s_instance = nullptr;

input* in = platform::in;

b8 application::application_create(game* game_inst) {
    if (initialized) {
        IC_ERROR("application create called more than once");
        return FALSE;
    }

    this->game_inst = game_inst;

    // Initialize Subsystems
    log.initialize_logging();
    in->input_initialize();

    IC_FATAL("A test message: %f", 3.14f);
    IC_ERROR("A test message: %f", 3.14f);
    IC_WARN("A test message: %f", 3.14f);
    IC_INFO("A test message: %f", 3.14f);
    IC_DEBUG("A test message: %f", 3.14f);
    IC_TRACE("A test message: %f", 3.14f);

    this->is_running = TRUE;
    this->is_suspended = FALSE;

    if (!event_initialize()) {
        IC_ERROR("Event System failed to initialize! Shutting down!");
        return FALSE;
    }

    // registers event
    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    if (!platform_startup(
        &this->state,
        game_inst->app_config.name,
        game_inst->app_config.start_position_x,
        game_inst->app_config.start_position_y,
        game_inst->app_config.start_width,
        game_inst->app_config.start_height
    )) { return FALSE; }

    // Initialize game
    if (!this->game_inst->initialize(this->game_inst)) {
        IC_FATAL("Game failed to initialize");
        return FALSE;
    }

    this->game_inst->on_resize(this->game_inst, this->width, this->height);

    initialized = TRUE;

    return TRUE;
}

b8 application::run() {
    IC_INFO(mem.get_memory_usage_str())
    
    while (this->is_running) {
        if (!platform_pump_messages(&this->state)) {
            this->is_running = FALSE;
        }

        // this chunk of code only gets called once this->suspended becomes -2 for some reason
        if (!this->is_suspended) {
            if (!this->game_inst->update(this->game_inst, (f32)0)) {
                IC_FATAL("Game update failed. Shutting down!");
                this->is_running = FALSE;
                break;
            }

            // game's rendering
            if (!this->game_inst->render(this->game_inst, (f32)0)) {
                IC_FATAL("Game rendering failed. Shutting down!");
                this->is_running = FALSE;
                break;
            }

            // NOTE: Input update/state copying should always be handled
            // after any input should be recorded; I.E. before this line.
            // As a safety, input is the last thing to be updated before
            // this frame ends.
            in->input_update(0);
        }

    }

    this->is_running = FALSE;

    // Shutdown event system.
    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
    
    event_shutdown();
    in->input_shutdown();

    platform_shutdown(&this->state);

    return TRUE;
}

application::application() {
    s_instance = this;
}

application::~application() {
}

// this function doesn't get called
b8 application::application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            IC_INFO("Application quit event recieved, shutting down.\n");
            s_instance->is_running = FALSE;
            return TRUE;
        }
    }

    return FALSE;
}

// this function doesn't get called
b8 application::application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            event_context data {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            return TRUE;
        } else if (key_code == KEY_A) {
            // Example on checking for a key
            IC_DEBUG("Explicit - A key pressed!");
        } else {
            IC_DEBUG("'%c' key pressed in window.", key_code);
        }

    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            // Example on checking for a key
            IC_DEBUG("Explicit - B key released!");
        } else {
            IC_DEBUG("'%c' key released in window.", key_code);
        }
    }
    return FALSE;
    
}