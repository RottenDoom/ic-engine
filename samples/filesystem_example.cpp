#include <ic_engine.h>

int main()
{
        ic::window_props w;
        ic_create_application(&w);

        /** This is an example usage of FileSystem API in ic_engine library */
        const char *user = ic_getuserdir();
        const char *base = ic_getbasedir();

        // Mount game directories
        ic_mount("/game_data/assets", "/assets", 1);
        ic_mount("/config", "/config", 2);
        ic_mount("/cube", "/cube", 3);

        // Check if a file exists
        if (ic_exists("/config/config.ini"))
                IC_TRACE("Config file exists");

        // // Create a directory (use ./ fo making the directory in base/app dir)
        // if (ic_mkdir("/game/saves"))
        // {
        //         IC_TRACE("Saves directory created");
        // }
        // else
        // {
        //         IC_TRACE("Could not create saves directory");
        // }

        if (ic_open("/cube/cube.gltf"))
        {
                size_t len;
                char  *file = ic_read("/cube/cube.gltf", &len);

                printf("File content:\n %s\n", file);
        }

        ic_app_destroy();
}
