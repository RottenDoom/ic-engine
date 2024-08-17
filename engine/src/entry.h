#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "game_inst.h"

// Tell the library that we externally created game instance
extern b8 create_game(game* out_game);

// main entrypoint for the engine application
int main(void) {
    // game intance
    game game_inst;
    if (!create_game(&game_inst)) {
        IC_FATAL("Could not create game!");
        return -1;
    }

    // ensure function pointers exists
    if (!game_inst.render || !game_inst.update || !game_inst.on_resize || !game_inst.initialize) {
        IC_FATAL("Game's function pointers not assigned!");
        return -2;
    }

    // initialization
    application app;
    if (!app.application_create(&game_inst)) {
        IC_INFO("Application failed to create!");
        return 1;
    }

    // begin loop
    if (!app.run()) {
        IC_INFO("Application did not shutdown gracefully!");
        return 2;
    }

    return 0;
}