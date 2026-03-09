#include <ic_engine.h>
#include <iostream>

struct ApplicationState
{
        ic::window_props *props;
};

struct GameState
{
        std::vector<GUID> models;
};

GameState g_state;

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
        const char      *title = "IC Engine test v0.02";
        state.props            = new ic::window_props(title);

        ic_create_application(state.props);
}

int main(int argc, char *argv[])
{
        createApplication();
        IC_INFO("Main Entrypoint");

        ic_app_set_callback(update, render);

        /** TODO:
         * 1. ICM or fast file loads
         * 2. Load the model with names and everything.
         * 3. Write files from gltf i.e create a converter for my project
         * 4. Do fast file loads and multi threading
         */

        // path after post-build
        ic_mount("/assets", "assets", 1);

        // load the registry (IN some functions you would have to put assets at the start in some you dont have
        // to)
        ic_load_registry("assets/registry.yaml");

        // load model
        GUID id = 0x1000000000000004;
        ic_load_model(id);
        g_state.models.push_back(id);

        ic::RenderScene defaultScene;
        ic::Entity      entt = defaultScene.createEntity();

        defaultScene.addMesh(entt).modelID       = id;
        defaultScene.addTransform(entt).position = {0, 0, 0};

        // Camera Entity

        ic_set_scene(&defaultScene);

        ic_app_run();

        ic_app_destroy();

        std::cin.get();
        return 0;
}