#include <ic_engine.h>

int main()
{
        ic::window_props w;
        ic_create_application(&w);

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

        ic_app_destroy();
}
