#pragma once
#include "renderer/model.h"
#include <glad/glad.h>

namespace ic
{
struct GLTexture
{
        GLuint textureHandle;
        void createTexture(Model& model, Texture& tex, ImageData& img);
        void applySampler(Sampler& sampler);
};

struct GLMaterial
{
};

}  // namespace ic