#include "game.h"

#include <entry.h>

// TODO remove this
#include <core/ic_memory.h>

// define game and assign function pointers

b8 create_game(game* out_game) {
    out_game->app_config.start_position_x = 100;
    out_game->app_config.start_position_y = 100;
    out_game->app_config.start_width = 1280;
    out_game->app_config.start_height = 720;

    out_game->app_config.name = (char*)"IC Engine Testbed";

    out_game->update = game_update;
    out_game->render = game_render;
    out_game->initialize = game_initialize;
    out_game->on_resize = game_on_resize;

    // Create game state
    // TODO remove platform memory allocation
    memory mem;
    out_game->state = mem.ic_allocate(sizeof(game_state), MEMORY_TAG_GAME);
    
    return TRUE;
}