#ifndef GL_SKYBOX_H
#define GL_SKYBOX_H

#include "core/assets/types/skybox.h"

namespace ic
{
class GLSkybox
{
public:
        GLSkybox() = default;
        ~GLSkybox() { Destroy(); }

        bool Upload(const Skybox &skybox_asset);
        void Destroy();
        void Bind(GLenum textureUnit = 0) const;

        bool     IsReady() const { return m_cubemapID != 0; }
        GLuint   GetCubemapID() const { return m_cubemapID; }
        uint32_t GetVAO() const { return m_vao; }

private:
        //	IDK about this
        void BuildCubeGeometry();

        GLuint m_cubemapID = 0;
        GLuint m_vao       = 0;
        GLuint m_vbo       = 0;
};

}  // namespace ic

#endif