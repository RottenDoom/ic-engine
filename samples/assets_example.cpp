#include <ic_engine.h>

int main()
{
        ic::window_props w;
        ic_create_application(&w);
        /** TODO: Inconsistency in filesystem gotta write some tests */

        // path after post-build
        IC_fs_mount("/assets", "assets", true);

        // load the registry (IN some functions you would have to put assets at the start in some you dont have to)
        ic_load_registry("registry.yaml");

        // load model
        GUID id = 0x1000000000000001;
        ic_load_model(id);

        ic_app_destroy();
}
