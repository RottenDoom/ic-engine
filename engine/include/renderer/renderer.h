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

// Abstract renderer interface - lives in core
class IRenderer
{
public:
        virtual ~IRenderer() = default;

        virtual bool init(Window *w)       = 0;
        virtual void onEvent(event &e)     = 0;
        virtual void renderFrame(float dt) = 0;
        virtual void cleanUp()             = 0;

        virtual void setScene(ICScene *scene) = 0;
};

// Factory function to create the renderer (implemented in engine-renderer)
IRenderer *createRenderer(RendererAPI api);
void       destroyRenderer(IRenderer *renderer);

}  // namespace ic

#ifdef __cplusplus
extern "C"
{
#endif

        IC_API void ic_set_scene(ICScene *scene);

#ifdef __cplusplus
}
#endif

#endif