#include <ic_engine.h>

int main()
{
        ic::window_props w;
        ic_create_application(&w);
        /** TODO: Inconsistency in filesystem gotta write some tests */

        // path after post-build
        ic_mount("/assets", "assets", 1);

        // load the registry (IN some functions you would have to put assets at the start in some you dont have to)
        ic_load_registry("registry.yaml");

        // load model
        ic_load_model("cube_model");

        ic_app_destroy();
}
