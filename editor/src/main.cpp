#include <ic_engine.h>
#include "editor.h"

ic::EditorSystem editor;

static void editor_update(float dt)
{
        editor.Update(dt);
}

static void editor_render()
{
        editor.Render();
}

int main()
{
        // Application Init
        ic::window_props props("IC Engine v0.4");
        ic_create_application(&props);

        ic_mount("/assets", "assets", 1); /** TODO: Make paths for different asset types */
        ic_load_registry("/assets/registry.yaml");

        // Editor initialization
        editor.Init();

        ic_app_set_callback(editor_update, editor_render);

        ic_app_run();

        editor.Shutdown();
        ic_app_destroy();

        return 0;
}
