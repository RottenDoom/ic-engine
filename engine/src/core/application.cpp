#include "application.h"
#include "game_inst.h"
#include <functional>

#include "logger.h"

#define BIND_EVENT_FN(x) std::bind(&application::x, this, std::placeholders::_1)

b8 application::application_create(game* game_inst) {
    if (initialized) {
        IC_ERROR("application create called more than once");
        return FALSE;
    }

    this->game_inst = game_inst; // maybe problem is here

    // Initialize Subsystems
    log.initialize_logging();
    m_input.input_initialoize();

    IC_FATAL("A test message: %f", 3.14f);
    IC_ERROR("A test message: %f", 3.14f);
    IC_WARN("A test message: %f", 3.14f);
    IC_INFO("A test message: %f", 3.14f);
    IC_DEBUG("A test message: %f", 3.14f);
    IC_TRACE("A test message: %f", 3.14f);

    this->is_running = TRUE;
    this->is_suspended = FALSE;

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
    
    i32 i = 0;
    while (this->is_running) {
        if (!platform_pump_messages(&this->state)) {
            this->is_running = FALSE;
        }

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
            m_input.input_update(0);
        }
    }

    this->is_running = FALSE;

    

    platform_shutdown(&this->state);

    return TRUE;
}

application::application() {
    
}

application::~application() {
}

b8 application::application_on_event(event& e) {
    event_dispatcher dispatcher(e);
    dispatcher.dispatch<window_close_event>(BIND_EVENT_FN(application_on_key));

    for (auto it = m_layer_stack.end(); it != m_layer_stack.begin();)
    {
        (*--it)->on_event(e);
        if (e.handled)
            break;
    }
}

b8 application::application_on_key(window_close_event& e) {
    return FALSE;
}