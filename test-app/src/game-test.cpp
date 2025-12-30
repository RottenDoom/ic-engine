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

        ic_app_run();

        ic_app_destroy();
}