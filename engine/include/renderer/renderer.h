#ifndef RENDERER_H
#define RENDERER_H

#include "defines.h"
#include "core/window.h"
#include "core/events/event.h"
#include "renderer/scene.h"

/*
 * The renderer class is nothing but a cross platformer frontend for my engine. I am writing this engine currently in
 * vulkan. My engine in probably also contain support for other graphic API (hopefully) and I would like this class to
 * be the main hub for all three to present their code here.
 */

namespace ic
{

enum class RendererAPI
{
        OpenGL,
        Vulkan,
        DXD13,
};

// TODO: Probably replace this with IGraphicsDevice
class IRenderer
{
public:
        virtual ~IRenderer() = default;

        virtual bool Init(Window *w)       = 0;
        virtual void OnEvent(event &e)     = 0;
        virtual void RenderFrame(float dt) = 0;
        virtual void CleanUp()             = 0;

        virtual void SetScene(RenderScene *scene) = 0;
};

// Factory function to create the renderer (implemented in engine-renderer)
IRenderer *create_renderer(RendererAPI api);
void       destroy_renderer(IRenderer *renderer);

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif

        IC_API void ic_set_scene(ic::RenderScene *scene);

#ifdef __cplusplus
}
#endif

#endif