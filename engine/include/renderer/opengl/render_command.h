#ifndef RENDER_COMMAND_H
#define RENDER_COMMAND_H

#include "defines.h"

#include <glm/glm.hpp>
#include <vector>

/** TODO: Maybe somehow abstract this directly into the engine someday. */

namespace ic
{

struct GLPrimitive;
struct GLMaterial;
struct GLModel;

// ---------------------------------------------------------------------------
// RenderCommand -> one primitive ready to be drawn
//
// Produced by OpenGLRenderer::Draw() for each scene primitive.
// Consumed by render passes.
// ---------------------------------------------------------------------------

struct RenderCommand
{
        GLPrimitive *primitive   = nullptr;
        GLMaterial  *material    = nullptr;  // null implies we use default flat material
        GLModel     *model       = nullptr;  // needed by passes to resolve textures
        glm::mat4    modelMatrix = glm::mat4(1.0f);
        float        depth       = 0.0f;  // world-space distance from camera
};

// ---------------------------------------------------------------------------
// RenderQueue -> per-frame command buckets
//
// submit() routes each command into one of three buckets based on material
// flags. sort() orders opaque front-to-back and blend back-to-front.
// ---------------------------------------------------------------------------

class RenderQueue
{
public:
        void Submit(RenderCommand cmd);
        void Sort();
        void Clear();

        const std::vector<RenderCommand> &OpaqueCommands() const { return m_opaque; }
        const std::vector<RenderCommand> &OutlineCommands() const { return m_outline; }
        const std::vector<RenderCommand> &BlendCommands() const { return m_blend; }

private:
        std::vector<RenderCommand> m_opaque;
        std::vector<RenderCommand> m_outline;
        std::vector<RenderCommand> m_blend;
};

}  // namespace ic

#endif  // RENDER_COMMAND_H
