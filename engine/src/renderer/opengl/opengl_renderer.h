#pragma once
#include "defines.h"

#include "core/window.h"
#include "core/events/event.h"

namespace ic
{
class OpenGLRenderer
{
private:
        void resizeCallback();
        void enableFeatures();
        // VBO VAO EBO etc
public:
        bool init(Window& window);
        void update(float deltaTime);  // Make a timestep module
        void onEvent(event& e);
        void draw();
        void destroy();

        OpenGLRenderer();
        ~OpenGLRenderer();
};
}  // namespace ic