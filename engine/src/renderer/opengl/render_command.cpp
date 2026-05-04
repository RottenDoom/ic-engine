#include "renderer/opengl/render_command.h"
#include "renderer/opengl/gl_material.h"

#include <algorithm>

namespace ic
{

void RenderQueue::Submit(RenderCommand cmd)
{
        if (!cmd.material)
        {
                m_opaque.push_back(cmd);
                return;
        }

        if (cmd.material->isInvertedHull)
                m_outline.push_back(cmd);
        else if (cmd.material->alphaMode == Material::AlphaMode::Blend)
                m_blend.push_back(cmd);
        else
                m_opaque.push_back(cmd);
}

void RenderQueue::Sort()
{
        // Opaque: front-to-back (ascending depth) reduces overdraw
        std::sort(m_opaque.begin(),
                  m_opaque.end(),
                  [](const RenderCommand &a, const RenderCommand &b) { return a.depth < b.depth; });

        // Blend: back-to-front (descending depth) for correct alpha compositing
        std::sort(m_blend.begin(),
                  m_blend.end(),
                  [](const RenderCommand &a, const RenderCommand &b) { return a.depth > b.depth; });

        // Outline: no sort needed (few items, stable order is fine)
}

void RenderQueue::Clear()
{
        m_opaque.clear();
        m_outline.clear();
        m_blend.clear();
}

}  // namespace ic
