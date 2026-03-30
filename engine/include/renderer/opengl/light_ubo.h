#pragma once
#include "renderer/light_types.h"
#include <glad/glad.h>

namespace ic
{

// This currently own gl objects as well as UBO I might make this something out in the main renderer later.
class LightUBO
{
public:
        LightUBO()  = default;
        ~LightUBO() = default;

        void Create();
        void Destroy();

        void UploadPerFrame(const PerFrameData &data);
        void UploadLights(const LightBlockData &data);

        // Call once after Create() so every shader that uses
        // std140 binding points finds the right buffer.
        void BindAll() const;

        static constexpr GLuint BINDING_PER_FRAME = 0;
        static constexpr GLuint BINDING_LIGHTS    = 1;

private:
        GLuint m_perFrameUBO = 0;
        GLuint m_lightUBO    = 0;

        void CreateUBO(GLuint &out, GLsizeiptr size, GLuint binding);
};

}  // namespace ic