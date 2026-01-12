#include <ic_engine.h>

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

        ic::test_bump_allocator();

        /** TODO: Remove all this  */
        /** This is an example usage of FileSystem API in ic_engine library */
        const char* user = IC_fs_getuserdir();
        const char* base = IC_fs_getbasedir();

        // Print system directories
        IC_TRACE("Base directory: {}", user);
        IC_TRACE("User directory: {}", base);

        // Mount game directories
        IC_fs_mount("/assets", "/game_data/assets", true);
        IC_fs_mount("/config", "/config", true);

        // Check if a file exists
        if (IC_fs_exists("config.ini"))
        {
                IC_TRACE("Config file exists");
        }
        else
        {
                IC_TRACE("Config file not found");
        }

        // Create a directory
        if (IC_fs_mkdir("./saves"))
        {
                IC_TRACE("Saves directory created");
        }
        // ==========================

        ic_app_run();

        ic_app_destroy();
}