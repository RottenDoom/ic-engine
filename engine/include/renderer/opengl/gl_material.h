#ifndef GL_MATERIAL_H
#define GL_MATERIAL_H

#include "defines.h"
#include "core/assets/types/model.h"
#include "core/assets/types/material.h"
#include "core/assets/types/texture.h"

#include "renderer/opengl/gl_sampler.h"

#include <glad/glad.h>
#include <vector>

namespace ic
{

class Shader;

// ---------------------------------------------------------------------------
// GLTexture -> one OpenGL texture object (owns the GL handle)
// Holds GL handles, asset handle and a copy of a sampler. the asset texture should contain
// ---------------------------------------------------------------------------

struct GLTexture
{
        GLuint        textureHandle = 0;
        TextureHandle assetHandle;  // sampler are inside texture asset
        GLSampler     sampler;      // copy of the texture asset sampler as its not a big deal to hold it for now

        void Reload();
        void Upload(const Image *img);
        void ApplySampler(const GLuint textureUnit) const;  // const: modifies GL state only
        void Destroy();

        bool IsValid() const { return textureHandle != 0; }
};

/** @brief Baked per draw material state. */
struct GLMaterial
{
        // Scalar uniforms
        glm::vec4 baseColorFactor   = glm::vec4(1.0f);
        float     metallicFactor    = 0.0f;
        float     roughnessFactor   = 1.0f;
        float     normalScale       = 1.0f;
        float     occlusionStrength = 1.0f;
        glm::vec3 emissiveFactor    = glm::vec3(0.0f);
        float     alphaCutoff       = 0.5f;

        // GPU texture handles that contain handles to Textures as well.
        GLTexture baseColor;
        GLTexture metallicRoughness;
        GLTexture normal;
        GLTexture occlusion;
        GLTexture emissive;

        // Render state flags
        Material::AlphaMode alphaMode   = Material::AlphaMode::Opaque;
        bool                doubleSided = false;

        // True when the material has no base-color or metallic-roughness
        // textures and is single-sided — signature of an inverted-hull outline mesh.
        bool isInvertedHull = false;

        /**
         * Bake all scalar fields and texture slot indices from a CPU Material.
         * Does not touch any GL objects.
         */
        void Build(const Material &mat);

        /**
         * Set all shader uniforms and bind texture units.
         * textures[] must be the owning GLModel's m_textures array.
         * samplers[] must be the source Model's samplers() array.
         */
        void Bind(Shader *shader) const;

        /**
         * Apply blend and cull-face GL state for this material.
         * The calling pass is responsible for restoring state afterwards.
         */
        void ApplyRenderState() const;
};

}  // namespace ic

#endif  // GL_MATERIAL_H