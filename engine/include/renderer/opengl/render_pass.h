#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "renderer/opengl/render_command.h"

namespace ic
{

class Shader;

// ---------------------------------------------------------------------------
// RenderPass -> one typed rendering pass
//
// Each pass owns its GL state setup and teardown.
// The pass leaves GL state in a neutral/default condition after execute().
// ---------------------------------------------------------------------------

struct RenderPass
{
        virtual void Execute(const std::vector<RenderCommand> &commands, Shader *shader) = 0;
        virtual ~RenderPass()                                                            = default;
};

// Opaque geometry: depth test on, depth write on, GL_BACK cull, no blend.
// Sorted front-to-back.
struct OpaquePass : RenderPass
{
        void Execute(const std::vector<RenderCommand> &commands, Shader *shader) override;
};

// Inverted-hull outline geometry: GL_FRONT cull so only silhouette edges are visible.
// Runs after OpaquePass so depth buffer correctly rejects inner faces.
struct OutlinePass : RenderPass
{
        void Execute(const std::vector<RenderCommand> &commands, Shader *shader) override;
};

// Alpha-blended geometry: blend on, depth write off, sorted back-to-front.
struct TransparentPass : RenderPass
{
        void Execute(const std::vector<RenderCommand> &commands, Shader *shader) override;
};

}  // namespace ic

#endif  // RENDER_PASS_H
