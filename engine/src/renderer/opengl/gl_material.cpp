#include "renderer/opengl/gl_material.h"

/**
 * gl_material.cpp
 *
 * GLTexture implementation.
 *
 * upload() always forces RGBA8 -> Image::pixels is guaranteed RGBA8
 * by the loader (stb_image forced to 4 channels). No format negotiation needed.
 *
 * Sampler enum values in Sampler::Filter and Sampler::Wrap intentionally
 * match GL constants, so the static_cast is a zero-cost mapping.
 */

namespace ic
{

void GLTexture::upload(const Image &img)
{
        if (img.pixels.empty() || img.width == 0 || img.height == 0)
        {
                IC_CORE_WARN("GLTexture::upload -> empty or zero-size image, skipping");
                return;
        }

        // Release previous handle if re-uploading
        destroy();

        glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

        // Allocate immutable storage -> RGBA8 always, full mip chain
        const GLsizei mipLevels = 1 + static_cast<GLsizei>(std::floor(std::log2(std::max(img.width, img.height))));

        glTextureStorage2D(textureHandle,
                           mipLevels,
                           img.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
                           static_cast<GLsizei>(img.width),
                           static_cast<GLsizei>(img.height));

        // Upload base mip level
        glTextureSubImage2D(textureHandle,
                            0,  // mip level
                            0,
                            0,  // x, y offset
                            static_cast<GLsizei>(img.width),
                            static_cast<GLsizei>(img.height),
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            img.pixels.data());

        glGenerateTextureMipmap(textureHandle);

        // Default filtering -> overridden by applySampler() if a sampler is present
        glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_REPEAT);

        IC_CORE_TRACE("GLTexture: uploaded {}x{} {} handle={}",
                      img.width,
                      img.height,
                      img.srgb ? "sRGB" : "linear",
                      textureHandle);
}

void GLTexture::applySampler(const Sampler *sampler)
{
        if (!isValid())
                return;

        // Sampler::Filter and Sampler::Wrap values match GL constants exactly.
        // NoFilter/NoWrap fall back to the defaults set in upload().

        if (sampler->minFilter != Sampler::Filter::None)
                glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(sampler->minFilter));

        if (sampler->magFilter != Sampler::Filter::None)
                glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(sampler->magFilter));

        if (sampler->wrapS != Sampler::Wrap::None)
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, static_cast<GLint>(sampler->wrapS));

        if (sampler->wrapT != Sampler::Wrap::None)
                glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, static_cast<GLint>(sampler->wrapT));
}

void GLTexture::destroy()
{
        if (textureHandle != 0)
        {
                glDeleteTextures(1, &textureHandle);
                textureHandle = 0;
        }
}

}  // namespace ic