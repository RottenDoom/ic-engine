#ifndef GL_MATERIAL_H
#define GL_MATERIAL_H

#include "core/assets/types/model.h"
#include "core/assets/types/material.h"
#include <glad/glad.h>

/** Gonna make a Material system here */
namespace ic
{
struct GLTexture
{
        GLuint textureHandle;
        void createTexture(Model &model, Texture &tex, ImageData &img);
        void applySampler(Sampler &sampler);
};
}  // namespace ic

#endif