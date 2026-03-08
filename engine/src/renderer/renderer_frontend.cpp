#include "renderer/renderer.h"
#include "renderer/scene.h"
#include "renderer/opengl/opengl_renderer.h"
// #include "renderer/vulkan/vulkan_renderer.h"  // When you add Vulkan

namespace ic
{

IRenderer *createRenderer(RendererAPI api)
{
        switch (api)
        {
        case RendererAPI::OpenGL:
                return new OpenGLRenderer();

        default:
                return nullptr;
                break;
        }
}

void destroyRenderer(IRenderer *renderer) {}

}  // namespace ic
