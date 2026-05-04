#include "renderer/opengl/gl_skybox.h"

#include <glad/glad.h>

static constexpr float k_cubeVertices[] = {-1, -1, -1, 1,  -1, -1, 1,  1,  -1, 1,  1,  -1, -1, 1,  -1, -1, -1, -1,
                                           -1, -1, 1,  1,  -1, 1,  1,  1,  1,  1,  1,  1,  -1, 1,  1,  -1, -1, 1,
                                           -1, 1,  1,  -1, 1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, 1,  -1, 1,  1,
                                           1,  1,  1,  1,  1,  -1, 1,  -1, -1, 1,  -1, -1, 1,  -1, 1,  1,  1,  1,
                                           -1, -1, -1, 1,  -1, -1, 1,  -1, 1,  1,  -1, 1,  -1, -1, 1,  -1, -1, -1,
                                           -1, 1,  -1, 1,  1,  -1, 1,  1,  1,  1,  1,  1,  -1, 1,  1,  -1, 1,  -1};

namespace ic
{
bool GLSkybox::Upload(const Skybox &skybox_asset)
{
        IC_CORE_ASSERT(skybox_asset.IsLoaded(), "GLSkybox::Upload -> asset not in Ready state");

        glGenTextures(1, &m_cubemapID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapID);

        static constexpr GLenum k_faceTargets[6] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                                                    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                                                    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                                                    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                                                    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                                                    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

        const auto &faces = skybox_asset.GetFaces();
        for (uint8_t i = 0; i < 6; i++)
        {
                const CubemapFace   &f      = faces[i];
                const CubemapFormat &format = skybox_asset.GetFormat();

                for (uint32_t level = 0; level < skybox_asset.GetMipLevels(); level++)
                {
                        const CubemapFaceLevel &mip = f.mips[level];
                        if (format.compressed)
                        {
                                glCompressedTexImage2D(k_faceTargets[i],
                                                       level,
                                                       format.internalFormat,
                                                       mip.width,
                                                       mip.height,
                                                       0,
                                                       static_cast<GLsizei>(mip.data.size()),
                                                       mip.data.data());
                        }
                        else
                        {
                                glTexImage2D(k_faceTargets[i],
                                             level,
                                             format.internalFormat,
                                             mip.width,
                                             mip.height,
                                             0,
                                             format.externalFormat,
                                             format.type,
                                             mip.data.data());
                        }
                }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                        skybox_asset.GetMipLevels() > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        BuildCubeGeometry();
        return true;
}

void GLSkybox::Bind(GLenum textureUnit) const
{
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapID);
}

void GLSkybox::BuildCubeGeometry()
{
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(k_cubeVertices), k_cubeVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

        glBindVertexArray(0);
}

void GLSkybox::Destroy()
{
        if (m_cubemapID)
        {
                glDeleteTextures(1, &m_cubemapID);
                m_cubemapID = 0;
        }
        if (m_vao)
        {
                glDeleteVertexArrays(1, &m_vao);
                m_vao = 0;
        }
        if (m_vbo)
        {
                glDeleteBuffers(1, &m_vbo);
                m_vbo = 0;
        }
}

}  // namespace ic