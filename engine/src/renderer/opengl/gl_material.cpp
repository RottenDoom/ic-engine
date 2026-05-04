#include "renderer/opengl/gl_material.h"
#include "renderer/opengl/gl_shader.h"

namespace ic
{

void GLTexture::Upload(const Image &img)
{
        if (img.pixels.empty() || img.width == 0 || img.height == 0)
        {
                IC_CORE_WARN("GLTexture::upload -> empty or zero-size image, skipping");
                return;
        }

        // Release previous handle if re-uploading
        Destroy();

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

void GLTexture::ApplySampler(const Sampler *sampler) const
{
        if (!IsValid())
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

void GLTexture::Destroy()
{
        if (textureHandle != 0)
        {
                glDeleteTextures(1, &textureHandle);
                textureHandle = 0;
        }
}

// ---------------------------------------------------------------------------
// GLMaterial
// ---------------------------------------------------------------------------

void GLMaterial::Build(const Material &mat)
{
        baseColorFactor   = mat.pbr.baseColorFactor;
        metallicFactor    = mat.pbr.metallicFactor;
        roughnessFactor   = mat.pbr.roughnessFactor;
        normalScale       = mat.normalTexture.scale;
        occlusionStrength = mat.occlusionTexture.strength;
        emissiveFactor    = mat.emissiveFactor;
        alphaCutoff       = mat.alphaCutoff;
        alphaMode         = mat.alphaMode;
        doubleSided       = mat.doubleSided;

        if (mat.pbr.baseColorTexture.isValid())
        {
                baseColorIdx     = static_cast<int>(mat.pbr.baseColorTexture.image);
                baseColorSampler = mat.pbr.baseColorTexture.sampler;
        }
        if (mat.pbr.metallicRoughnessTexture.isValid())
        {
                metallicRoughIdx     = static_cast<int>(mat.pbr.metallicRoughnessTexture.image);
                metallicRoughSampler = mat.pbr.metallicRoughnessTexture.sampler;
        }
        if (mat.normalTexture.isValid())
        {
                normalIdx     = static_cast<int>(mat.normalTexture.ref.image);
                normalSampler = mat.normalTexture.ref.sampler;
        }
        if (mat.occlusionTexture.isValid())
        {
                occlusionIdx     = static_cast<int>(mat.occlusionTexture.ref.image);
                occlusionSampler = mat.occlusionTexture.ref.sampler;
        }
        if (mat.emissiveTexture.isValid())
        {
                emissiveIdx     = static_cast<int>(mat.emissiveTexture.image);
                emissiveSampler = mat.emissiveTexture.sampler;
        }

        isInvertedHull = !mat.pbr.baseColorTexture.isValid() && !mat.pbr.metallicRoughnessTexture.isValid() &&
                         !mat.doubleSided;
}

void GLMaterial::Bind(Shader                       *shader,
                      const std::vector<GLTexture> &textures,
                      const std::vector<Sampler>   &samplers) const
{
        shader->setBool("u_useDefaultMaterial", false);

        // --- Base color ---
        shader->setVec4("u_BaseColorFactor", baseColorFactor);
        if (baseColorIdx >= 0 && static_cast<size_t>(baseColorIdx) < textures.size() &&
            textures[baseColorIdx].IsValid())
        {
                shader->setTexture("u_BaseColorTexture", 0, textures[baseColorIdx].textureHandle);
                if (baseColorSampler != INVALID_INDEX && baseColorSampler < samplers.size())
                        textures[baseColorIdx].ApplySampler(&samplers[baseColorSampler]);
                shader->setBool("u_HasBaseColorTexture", true);
        }
        else
        {
                shader->setBool("u_HasBaseColorTexture", false);
        }

        // --- Metallic / roughness ---
        shader->setFloat("u_MetallicFactor", metallicFactor);
        shader->setFloat("u_RoughnessFactor", roughnessFactor);
        if (metallicRoughIdx >= 0 && static_cast<size_t>(metallicRoughIdx) < textures.size() &&
            textures[metallicRoughIdx].IsValid())
        {
                shader->setTexture("u_MetallicRoughnessTexture", 1, textures[metallicRoughIdx].textureHandle);
                if (metallicRoughSampler != INVALID_INDEX && metallicRoughSampler < samplers.size())
                        textures[metallicRoughIdx].ApplySampler(&samplers[metallicRoughSampler]);
                shader->setBool("u_HasMetallicRoughnessTexture", true);
        }
        else
        {
                shader->setBool("u_HasMetallicRoughnessTexture", false);
        }

        // --- Normal ---
        if (normalIdx >= 0 && static_cast<size_t>(normalIdx) < textures.size() && textures[normalIdx].IsValid())
        {
                shader->setTexture("u_NormalTexture", 2, textures[normalIdx].textureHandle);
                if (normalSampler != INVALID_INDEX && normalSampler < samplers.size())
                        textures[normalIdx].ApplySampler(&samplers[normalSampler]);
                shader->setFloat("u_NormalScale", normalScale);
                shader->setBool("u_HasNormalTexture", true);
        }
        else
        {
                shader->setBool("u_HasNormalTexture", false);
        }

        // --- Occlusion ---
        if (occlusionIdx >= 0 && static_cast<size_t>(occlusionIdx) < textures.size() &&
            textures[occlusionIdx].IsValid())
        {
                shader->setTexture("u_OcclusionTexture", 3, textures[occlusionIdx].textureHandle);
                if (occlusionSampler != INVALID_INDEX && occlusionSampler < samplers.size())
                        textures[occlusionIdx].ApplySampler(&samplers[occlusionSampler]);
                shader->setFloat("u_OcclusionStrength", occlusionStrength);
                shader->setBool("u_HasOcclusionTexture", true);
        }
        else
        {
                shader->setBool("u_HasOcclusionTexture", false);
        }

        // --- Emissive ---
        shader->setVec3("u_EmissiveFactor", emissiveFactor);
        if (emissiveIdx >= 0 && static_cast<size_t>(emissiveIdx) < textures.size() && textures[emissiveIdx].IsValid())
        {
                shader->setTexture("u_EmissiveTexture", 4, textures[emissiveIdx].textureHandle);
                if (emissiveSampler != INVALID_INDEX && emissiveSampler < samplers.size())
                        textures[emissiveIdx].ApplySampler(&samplers[emissiveSampler]);
                shader->setBool("u_HasEmissiveTexture", true);
        }
        else
        {
                shader->setBool("u_HasEmissiveTexture", false);
        }

        shader->setFloat("u_AlphaCutoff", alphaCutoff);
}

void GLMaterial::ApplyRenderState() const
{
        if (alphaMode == Material::AlphaMode::Blend)
        {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
                glDisable(GL_BLEND);
        }

        if (doubleSided)
        {
                glDisable(GL_CULL_FACE);
        }
        else
        {
                glEnable(GL_CULL_FACE);
                glCullFace(isInvertedHull ? GL_FRONT : GL_BACK);
        }
}

}  // namespace ic