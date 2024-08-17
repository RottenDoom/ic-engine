#include "application.h"
#include "game_inst.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/ic_memory.h"

// static variables only stay inside this file
static b8 initialized = FALSE;
static application app;
static memory mem;

b8 application::application_create(game* game_inst) {
    if (initialized) {
        IC_ERROR("application create called more than once");
        return FALSE;
    }

    app.game_inst = game_inst;

    // Initialize Subsystems
    Logger log;
    log.initialize_logging();

    IC_FATAL("A test message: %f", 3.14f);
    IC_ERROR("A test message: %f", 3.14f);
    IC_WARN("A test message: %f", 3.14f);
    IC_INFO("A test message: %f", 3.14f);
    IC_DEBUG("A test message: %f", 3.14f);
    IC_TRACE("A test message: %f", 3.14f);

    app.is_running = TRUE;
    app.is_suspended = FALSE;

    if (!platform_startup(
        &app.state,
        game_inst->app_config.name,
        game_inst->app_config.start_position_x,
        game_inst->app_config.start_position_y,
        game_inst->app_config.start_width,
        game_inst->app_config.start_height
    )) { return FALSE; }

    // Initialize game
    if (!app.game_inst->initialize(app.game_inst)) {
        IC_FATAL("Game failed to initialize");
        return FALSE;
    }

    app.game_inst->on_resize(app.game_inst, app.width, app.height);

    initialized = TRUE;

    return TRUE;
}
b8 application::run() {
    IC_INFO(mem.get_memory_usage_str())
    while (app.is_running) {
        if (!platform_pump_messages(&app.state)) {
            app.is_running = FALSE;
        }

        if (!app.is_suspended) {
            if (!app.game_inst->update(app.game_inst, (f32)0)) {
                IC_FATAL("Game update failed. Shutting down!");
                app.is_running = FALSE;
                break;
            }

            // game's rendering
            if (!app.game_inst->render(app.game_inst, (f32)0)) {
                IC_FATAL("Game rendering failed. Shutting down!");
                app.is_running = FALSE;
                break;
            }
        }
    }

    app.is_running = FALSE;

    platform_shutdown(&app.state);

    return TRUE;
}