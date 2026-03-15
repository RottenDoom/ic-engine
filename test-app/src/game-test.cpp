#include <ic_engine.h>
#include <iostream>

struct ApplicationState
{
        ic::window_props *props;
};

struct GameState
{
        std::vector<GUID> models;
        ic::RenderScene   defaultScene;  // TODO Probably gonna make a World class later instead of directly using scene
                                         // here.
};

GameState g_state;

struct PlayerComponent
{
        float   speed  = 5.0f;
        uint8_t health = 100;
};

/** User side update and render functions */
void update(float deltaTime) /** TODO: add user side time update functions or udata pointer */
{
        g_state.defaultScene.Each<PlayerComponent, ic::TransformComponent>(
            [&](auto entity, PlayerComponent &p, ic::TransformComponent &t)
            {
                    if (ic_input_key_pressed(ic::Key::W))
                            t.position.y += p.speed * deltaTime;
            });
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
         * 2. Load the model with names and everything.
         * 4. Do multi threading
         */

        // path after post-build
        ic_mount("/assets", "assets", 1);

        // load the registry
        ic_load_registry("assets/registry.yaml");

        // load model
        GUID id = 0x1000000000000004;
        ic_load_model(id);  // make so that this thing calls by name of the mesh.
                            // story the id provided for now I am storying in some variable. like playerModel;

        ic::Entity entt = g_state.defaultScene.CreateEntityWithName("Player");
        entt.AddComponent<PlayerComponent>();
        entt.GetComponent<ic::TransformComponent>().SetPosition({0.0f, 0.0f, 0.0f});
        entt.AddComponent<ic::MeshComponent>().SetMesh(id);  // this id that is output must be from

        // Camera Entity

        ic_set_scene(&g_state.defaultScene);  // TODO: this should be done internally

        ic_app_run();

        ic_app_destroy();

        std::cin.get();
        return 0;
}