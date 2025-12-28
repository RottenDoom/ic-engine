#include "game.h"
#include "renderer/asset_manager.h"

bool game_initialize(game* game_inst)
{
        game_state* state = (game_state*)malloc(sizeof(game_state));
        memset(state, 0, sizeof(game_state));

        game_inst->state = state;

        /** TODO: this is game_inst asset */
        game_inst->playerModel = ic::asset::LoadModel("shibahu/scenc.gltf");

        if (game_inst->playerModel == -1) /** TODO: get the invalid model handle here too */
        {
                IC_ERROR("The model handle genrated was invalid");
                return false;
        }
        // state->level_model  = ic::asset::LoadModel("assets/level.gltf");

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

        // ic::renderer::drawaModel(state->level_model);
        ic::asset::DrawModel(game_inst->state->player_model);

        // update model????
        return true;
}

void game_on_resize(game* game_inst, unsigned int width, unsigned int height) {}