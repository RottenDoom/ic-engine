#include "renderer/opengl/light_ubo.h"

namespace ic
{

void LightUBO::Create()
{
        CreateUBO(m_perFrameUBO, sizeof(PerFrameData), BINDING_PER_FRAME);
        CreateUBO(m_lightUBO, sizeof(LightBlockData), BINDING_LIGHTS);
}

void LightUBO::Destroy()
{
        glDeleteBuffers(1, &m_perFrameUBO);
        glDeleteBuffers(1, &m_lightUBO);
        m_perFrameUBO = m_lightUBO = 0;
}

void LightUBO::CreateUBO(GLuint &out, GLsizeiptr size, GLuint binding)
{
        glGenBuffers(1, &out);
        glBindBuffer(GL_UNIFORM_BUFFER, out);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, out);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightUBO::UploadPerFrame(const PerFrameData &data)
{
        glBindBuffer(GL_UNIFORM_BUFFER, m_perFrameUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PerFrameData), &data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightUBO::UploadLights(const LightBlockData &data)
{
        glBindBuffer(GL_UNIFORM_BUFFER, m_lightUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightBlockData), &data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightUBO::BindAll() const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_PER_FRAME, m_perFrameUBO);
        glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_LIGHTS, m_lightUBO);
}

}  // namespace ic