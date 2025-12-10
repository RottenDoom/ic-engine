#pragma once
#include "defines.h"

#include "core/window.h"

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
        void draw();

        OpenGLRenderer();
        ~OpenGLRenderer();
};
}  // namespace ic