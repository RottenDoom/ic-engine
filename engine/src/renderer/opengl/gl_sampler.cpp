#include "renderer/opengl/gl_sampler.h"

namespace ic
{

void GLSampler::Build(const Sampler &s)
{
        glCreateSamplers(1, &handle);
        if (s.minFilter != Sampler::Filter::None)
                glSamplerParameteri(handle, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(s.minFilter));
        if (s.magFilter != Sampler::Filter::None)
                glSamplerParameteri(handle, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(s.magFilter));
        if (s.wrapS != Sampler::Wrap::None)
                glSamplerParameteri(handle, GL_TEXTURE_WRAP_S, static_cast<GLint>(s.wrapS));
        if (s.wrapT != Sampler::Wrap::None)
                glSamplerParameteri(handle, GL_TEXTURE_WRAP_T, static_cast<GLint>(s.wrapT));
}

void GLSampler::Destroy()
{
        if (handle)
        {
                glDeleteSamplers(1, &handle);
                handle = 0;
        }
}

}  // namespace ic
