#include "renderer/opengl/render_pass.h"
#include "renderer/opengl/gl_material.h"
#include "renderer/opengl/gl_model.h"
#include "renderer/opengl/gl_shader.h"

#include <glad/glad.h>

namespace ic
{

// ---------------------------------------------------------------------------
// Shared draw helper
// ---------------------------------------------------------------------------

static void DispatchCommand(const RenderCommand &cmd, Shader *shader)
{
        shader->setMat4("u_Model", cmd.modelMatrix);

        if (cmd.material && cmd.model && cmd.model->Get())
        {
                cmd.material->Bind(shader, cmd.model->samplers());
                cmd.material->ApplyRenderState();
        }
        else
        {
                // No material: draw with default flat shading
                shader->setBool("u_useDefaultMaterial", true);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glDisable(GL_BLEND);
        }

        if (!cmd.primitive || cmd.primitive->draw.count == 0)
                return;

        glBindVertexArray(cmd.primitive->VAO);
        const size_t idxSz = (cmd.primitive->indexType == GL_UNSIGNED_SHORT) ? 2 : 4;
        glDrawElementsBaseVertex(GL_TRIANGLES,
                                 cmd.primitive->draw.count,
                                 cmd.primitive->indexType,
                                 reinterpret_cast<void *>(cmd.primitive->draw.firstIndex * idxSz),
                                 cmd.primitive->draw.baseVertex);
}

// ---------------------------------------------------------------------------
// OpaquePass
// ---------------------------------------------------------------------------

void OpaquePass::Execute(const std::vector<RenderCommand> &commands, Shader *shader)
{
        if (commands.empty())
                return;

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        for (const RenderCommand &cmd : commands)
                DispatchCommand(cmd, shader);

        // Restore neutral cull state
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
}

// ---------------------------------------------------------------------------
// OutlinePass
// ---------------------------------------------------------------------------

void OutlinePass::Execute(const std::vector<RenderCommand> &commands, Shader *shader)
{
        if (commands.empty())
                return;

        // applyRenderState() on each inverted-hull material sets GL_FRONT cull
        for (const RenderCommand &cmd : commands)
                DispatchCommand(cmd, shader);

        // Restore
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
}

// ---------------------------------------------------------------------------
// TransparentPass
// ---------------------------------------------------------------------------

void TransparentPass::Execute(const std::vector<RenderCommand> &commands, Shader *shader)
{
        if (commands.empty())
                return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (const RenderCommand &cmd : commands)
                DispatchCommand(cmd, shader);

        // Restore
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
}

}  // namespace ic
