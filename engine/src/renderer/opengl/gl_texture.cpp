#include "gl_texture.h"

namespace ic
{

void GLTexture::createTexture(Model& model, Texture& tex, ImageData& img)
{
        glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

        // Determine internal format
        GLenum internalFormat = GL_RGBA8;
        GLenum format         = GL_RGBA;

        if (img.srgb)
        {
                internalFormat = GL_SRGB8_ALPHA8;
        }

        // Allocate storage
        glTextureStorage2D(textureHandle, 1, internalFormat, img.width, img.height);

        // Upload pixel data
        glTextureSubImage2D(textureHandle, 0, 0, 0, img.width, img.height, format, GL_UNSIGNED_BYTE, img.pixels.data());

        // Set sampler parameters
        if (tex.sampler != INVALID_INDEX)
        {
                Sampler& sampler = model.samplers[tex.sampler];

                glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(sampler.minFilter));
                glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(sampler.magFilter));
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, static_cast<GLint>(sampler.wrapS));
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, static_cast<GLint>(sampler.wrapT));
        }
        else
        {
                // Default sampler settings
                glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        // Generate mipmaps
        glGenerateTextureMipmap(textureHandle);
}

void GLTexture::applySampler(Sampler& sampler) {}

}  // namespace ic
