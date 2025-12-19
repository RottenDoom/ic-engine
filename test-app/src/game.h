#pragma once

#include "defines.h"
class game;

struct game_state
{
        // ic::Model playerModel;
        // ic::Model levelModel;

        float time;
};

bool game_initialize(game* game_inst);

bool game_update(game* game_inst, float delta_time);

bool game_render(game* game_inst, float delta_time);

void game_on_resize(game* game_inst, unsigned int width, unsigned int height);