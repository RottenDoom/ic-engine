#pragma once
#include "core/logger.h"
#include "core/application.h"
// should contain the declarations of the game class

class game
{
public:
        bool (*initialize)(class game* game_inst);
        bool (*update)(class game* game_inst, float delta_time);
        bool (*render)(class game* game_inst, float delta_time);
        void (*on_resize)(class game* game_inst, unsigned int width, unsigned int height);

        void* state;
};

extern bool create_game(game* out_game);

int main(void)
{
        game game_inst;
        if (!create_game(&game_inst))
        {
                IC_CORE_CRITICAL("Could not create game!");
                return -1;
        }

        // ensure function pointers exists
        if (!game_inst.render || !game_inst.update || !game_inst.on_resize || !game_inst.initialize)
        {
                IC_CORE_CRITICAL("Game's function pointers not assigned!");
                return -2;
        }

        // initialization
        ic::application app;
        if (!app.applicationCreate(&game_inst))
        {
                IC_CORE_ERROR("Application failed to create!");
                return 1;
        }

        // begin loop
        if (!app.run())
        {
                IC_CORE_ERROR("Application did not shutdown gracefully!");
                return 2;
        }

        IC_CORE_INFO("Application Shutdown Complete. Press Enter to exit...");
        std::cout.flush();
        std::cin.get();

        return 0;
}
