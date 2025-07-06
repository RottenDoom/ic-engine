#pragma once
#include "defines.h"
#include "core/Application.h"
// should contain the declarations of the game class

class game
{
    public:
    bool (*initialize)(class game* game_inst);
    bool (*update)(class game* game_inst, f32 delta_time);
    bool (*render)(class game* game_inst, f32 delta_time);
    void (*on_resize)(class game* game_inst, unsigned int width, unsigned int height);
    
    void* state;
};

extern bool create_game(game* out_game);

int main(void)
{
    game game_inst;
    if (!create_game(&game_inst)) {
        // IC_FATAL("Could not create game!");
        return -1;
    }
    
    // ensure function pointers exists
    if (!game_inst.render || !game_inst.update || !game_inst.on_resize || !game_inst.initialize) {
        // IC_FATAL("Game's function pointers not assigned!");
        return -2;
    }
    
    // initialization
    ic::application app;
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
