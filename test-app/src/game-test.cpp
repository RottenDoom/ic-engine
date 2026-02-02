#include <ic_engine.h>
#include <iostream>

struct ApplicationState
{
        ic::window_props* props;
        /** TODO: add more properties here */
};

struct GameState
{
};

/** User side update and render functions */
void update(float deltaTime) /** TODO: add user side time update functions or udata pointer */
{
        if (ic_input_key_pressed(ic::Key::G))
        {
                IC_INFO("G pressed");
        }
}

void render() {}

void createApplication()
{
        ApplicationState state;
        const char* title = "IC Engine test v0.02";
        state.props       = new ic::window_props(title);

        ic_create_application(state.props);
}

int main(int argc, char* argv[])
{
        createApplication();
        IC_INFO("Main Entrypoint");

        ic_app_set_callback(update, render);

        /** TODO: Remove all this  */
        /** This is an example usage of FileSystem API in ic_engine library */
        const char* user = IC_fs_getuserdir();
        const char* base = IC_fs_getbasedir();

        // Mount game directories
        IC_fs_mount("/game_data/assets", "/assets", true);
        IC_fs_mount("/config", "/config", true);
        IC_fs_mount("/cube", "/cube", true);

        // Check if a file exists
        if (IC_fs_exists("/config/config.ini"))
                IC_TRACE("Config file exists");

        // Create a directory (use ./ fo making the directory in base/app dir)
        if (IC_fs_mkdir("/game/saves"))
        {
                IC_TRACE("Saves directory created");
        }
        else
        {
                IC_TRACE("Could not create saves directory");
        }

        if (IC_fs_open("/cube/bullcrap.gltf"))
        {
                size_t len;
                char* file = IC_fs_read("/cube/bullcrap.gltf", &len);

                printf("File content:\n %s\n", file);
        }
        // ==========================

        ic_app_run();

        ic_app_destroy();

        std::cin.get();
        return 0;
}