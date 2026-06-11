#include "renderer/opengl/gl_material.h"
#include "renderer/opengl/gl_shader.h"

#include "core/assets/asset_manager.h"

namespace ic
{

static void build_texture(GLTexture &glTexture, const TextureHandle handle, AssetManager &mgr)
{
        if (handle == INVALID_ID)
                return;
        Texture *asset = mgr.GetAsset<Texture>(handle);
        Image   *img   = asset->GetImageTexture();
        glTexture.Upload(img);
        glTexture.assetHandle = handle;
        Sampler s             = asset->GetImageSampler();
        glTexture.sampler.Build(s);
}

void GLTexture::Upload(const Image *img)
{

        if (img->pixels.empty() || img->width == 0 || img->height == 0)
        {
                IC_CORE_WARN("GLTexture::Upload -> empty or zero-size image, skipping");
                return;
        }

        // Release previous handle if re-uploading
        Destroy();

        glCreateTextures(GL_TEXTURE_2D, 1, &textureHandle);

        // Allocate immutable storage -> RGBA8 always, full mip chain
        const GLsizei mipLevels = 1 + static_cast<GLsizei>(std::floor(std::log2(std::max(img->width, img->height))));

        glTextureStorage2D(textureHandle,
                           mipLevels,
                           img->srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8,
                           static_cast<GLsizei>(img->width),
                           static_cast<GLsizei>(img->height));

        // Upload base mip level
        glTextureSubImage2D(textureHandle,
                            0,  // mip level
                            0,
                            0,  // x, y offset
                            static_cast<GLsizei>(img->width),
                            static_cast<GLsizei>(img->height),
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            img->pixels.data());

        glGenerateTextureMipmap(textureHandle);

        // Default filtering -> overridden by applySampler() if a sampler is present
        glTextureParameteri(textureHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(textureHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureHandle, GL_TEXTURE_WRAP_T, GL_REPEAT);

        IC_CORE_TRACE("GLTexture: uploaded {}x{} {} handle={}",
                      img->width,
                      img->height,
                      img->srgb ? "sRGB" : "linear",
                      textureHandle);
}

void GLTexture::ApplySampler(const GLuint textureUnit) const
{
        glBindSampler(textureUnit, sampler.handle);
}

void GLTexture::Destroy()
{
        if (textureHandle != 0)
        {
                glDeleteTextures(1, &textureHandle);
                textureHandle = 0;
        }
        if (sampler.handle != 0)
        {
                glDeleteSamplers(1, &sampler.handle);
                sampler.handle = 0;
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
        emissiveFactor    = mat.emissiveTexture.emissiveFactor;
        alphaCutoff       = mat.alphaCutoff;
        alphaMode         = mat.alphaMode;
        doubleSided       = mat.doubleSided;

        auto &mgr = ic::AssetManager::Get();
        build_texture(baseColor, mat.pbr.baseColorTexture, mgr);
        build_texture(metallicRoughness, mat.pbr.metallicRoughnessTexture, mgr);
        build_texture(normal, mat.normalTexture.ref, mgr);
        build_texture(occlusion, mat.occlusionTexture.ref, mgr);
        build_texture(emissive, mat.emissiveTexture.emissiveTexture, mgr);

        isInvertedHull = !baseColor.IsValid() && !metallicRoughness.IsValid() && !mat.doubleSided;
}

/** TODO: what if a texture does not have a sampler */
void GLMaterial::Bind(Shader *shader) const
{
        shader->setBool("u_useDefaultMaterial", false);

        // --- Base color ---
        shader->setVec4("u_BaseColorFactor", baseColorFactor);
        if (baseColor.IsValid())
        {
                shader->setTexture("u_BaseColorTexture", 0, baseColor.textureHandle);
                baseColor.ApplySampler(0);
                shader->setBool("u_HasBaseColorTexture", true);
        }
        else
        {
                shader->setBool("u_HasBaseColorTexture", false);
        }

        // --- Metallic / roughness ---
        shader->setFloat("u_MetallicFactor", metallicFactor);
        shader->setFloat("u_RoughnessFactor", roughnessFactor);
        if (metallicRoughness.IsValid())
        {
                shader->setTexture("u_MetallicRoughnessTexture", 1, metallicRoughness.textureHandle);
                metallicRoughness.ApplySampler(1);
                shader->setBool("u_HasMetallicRoughnessTexture", true);
        }
        else
        {
                shader->setBool("u_HasMetallicRoughnessTexture", false);
        }

        // --- Normal ---
        if (normal.IsValid())
        {
                shader->setTexture("u_NormalTexture", 2, normal.textureHandle);
                normal.ApplySampler(2);
                shader->setFloat("u_NormalScale", normalScale);
                shader->setBool("u_HasNormalTexture", true);
        }
        else
        {
                shader->setBool("u_HasNormalTexture", false);
        }

        // --- Occlusion ---
        if (occlusion.IsValid())
        {
                shader->setTexture("u_OcclusionTexture", 3, occlusion.textureHandle);
                occlusion.ApplySampler(3);
                ;
                shader->setFloat("u_OcclusionStrength", occlusionStrength);
                shader->setBool("u_HasOcclusionTexture", true);
        }
        else
        {
                shader->setBool("u_HasOcclusionTexture", false);
        }

        // --- Emissive ---
        shader->setVec3("u_EmissiveFactor", emissiveFactor);
        if (emissive.IsValid())
        {
                shader->setTexture("u_EmissiveTexture", 4, emissive.textureHandle);
                emissive.ApplySampler(4);
                ;
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