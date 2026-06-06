#ifndef GL_SAMPLER_H
#define GL_SAMPLER_H

#include "defines.h"
#include "core/assets/types/model.h"
#include <glad/glad.h>

namespace ic
{

struct GLSampler
{
        GLuint handle = 0;

        void Build(const Sampler &s);
        void Destroy();
};

}  // namespace ic

#endif  // GL_SAMPLER_H