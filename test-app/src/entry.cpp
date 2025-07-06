#include "game.h"
#include "entry.h"

bool create_game(game* out_game)
{
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->initialize = game_initialize;
    out_game->on_resize = game_on_resize;

    out_game->state = nullptr;

    return true;
}