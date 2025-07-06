#pragma once

#include "defines.h"
class game;

bool game_initialize(game* game_inst);

bool game_update(game* game_inst, f32 delta_time);

bool game_render(game* game_inst, f32 delta_time);

void game_on_resize(game* game_inst, unsigned int width, unsigned int height);