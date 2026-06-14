#include "core/assets/types/material.h"

// GL_SPECIFIC

#include "renderer/opengl/gl_material.h"

namespace ic
{

void MaterialAsset::SetGPUHandle(GLMaterial *handle)
{
        m_gpu = handle;
}

void MaterialAsset::Rebake()
{
        if (!m_gpu)
                m_gpu = new GLMaterial();
        m_gpu->Build(m_material);
        m_dirty = false;
}

bool MaterialAsset::Release()
{
        delete m_gpu;
        return true;
}

}  // namespace ic
