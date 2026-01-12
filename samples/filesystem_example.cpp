#include <ic_engine.h>

int main()
{
        ic::window_props w;
        ic_create_application(&w);

        /** TODO: Remove all this  */
        /** This is an example usage of FileSystem API in ic_engine library */
        const char* user = IC_fs_getuserdir();
        const char* base = IC_fs_getbasedir();

        // Print system directories
        IC_TRACE("Base directory: {}", user);
        IC_TRACE("User directory: {}", base);

        // Mount game directories
        IC_fs_mount("/assets", "./game_data/assets", true);
        IC_fs_mount("/config", "./config", true);

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

        ic_app_destroy();
}
