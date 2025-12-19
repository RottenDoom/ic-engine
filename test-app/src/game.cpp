#include "game.h"

bool game_initialize(game* game_inst)
{
        game_state* state = (game_state*)malloc(sizeof(game_state));
        memset(state, 0, sizeof(game_state));

        // game_inst->state    = state;

        // state->player_model = ic::Assets::LoadModel("assets/player.glb");
        // state->level_model  = ic::Assets::LoadModel("assets/level.gltf");

        return true;
}

bool game_update(game* game_inst, float delta_time)
{
        // game_state* state = (game_state*)gama_inst->state;
        // state->time += delta_time;

        // gameplaylogic
        return true;
}

bool game_render(game* game_inst, float delta_time)
{
        // game_state* state = (game_state*)game_inst->state;

        // ic::renderer::drawModel(state->level_model);
        // ic::renderer::drawModel(state->player_model);

        // update model????
        return true;
}

void game_on_resize(game* game_inst, unsigned int width, unsigned int height) {}