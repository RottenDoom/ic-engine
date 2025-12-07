#pragma once
#include "defines.h"

#include "core/window.h"

namespace ic
{
class OpenGLRenderer
{
private:
        void resize_callback();
        // VBO VAO EBO etc
public:
        bool init(Window& window);
        void update(float deltaTime);  // Make a timestep module
        void draw();

        OpenGLRenderer();
        ~OpenGLRenderer();
};
}  // namespace ic