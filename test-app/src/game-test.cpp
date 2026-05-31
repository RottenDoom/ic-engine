#include <ic_engine.h>
#include <iostream>

struct ApplicationState
{
        ic::window_props *props;
};

struct GameState
{
        std::vector<IC_GUID> models;
        ic::RenderScene      defaultScene{"Scene"};  // TODO Probably gonna make a World class later instead of directly
                                                     // using scene here.
};

GameState g_state;

struct PlayerComponent
{
        float   speed  = 5.0f;
        uint8_t health = 100;
};

/** User side update and render functions */
void update(float deltaTime)
{
        ic::Entity              player = g_state.defaultScene.FindEntityByName("Player");
        PlayerComponent        &p      = player.GetComponent<PlayerComponent>();
        ic::TransformComponent &t      = player.GetComponent<ic::TransformComponent>();

        if (ic_input_key_pressed(ic::Key::W))
        {
                t.SetPosition(t.position + glm::vec3(0.0f, p.speed * deltaTime, 0.0f));
                IC_CORE_INFO("{}, {}, {}", t.position.x, t.position.y, t.position.z);
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
         * 4. Do multi threading
         */

        // path after post-build
        ic_mount("/assets", "assets", 1);

        // load the registry
        ic_load_registry("assets/registry.yaml");

        IC_GUID id = ic_load_model(
            "player_model");  // make so that this thing calls by name of the mesh.
                              // story the id provided for now I am storying in some variable. like playerModel;

        ic::Entity entt = g_state.defaultScene.CreateEntityWithName("Player");
        entt.AddComponent<PlayerComponent>();
        entt.GetComponent<ic::TransformComponent>().SetPosition({0.0f, 0.0f, 0.0f});
        entt.AddComponent<ic::MeshComponent>().SetModel(id);  // this id that is output must be from

        IC_GUID cube = ic_load_model("cube_model");

        ic::Entity cube_entt = g_state.defaultScene.CreateEntityWithName("Cube");
        cube_entt.GetComponent<ic::TransformComponent>().SetPosition({15.0f, 15.0f, 0.0f});
        cube_entt.AddComponent<ic::MeshComponent>().SetModel(cube);

        // Camera Entity

        // TODO: fix
        // ic_set_scene(&g_state.defaultScene);  // TODO: this should be done internally

        ic_app_run();

        ic_app_destroy();

        std::cin.get();
        return 0;
}