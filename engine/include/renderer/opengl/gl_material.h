#ifndef GL_MATERIAL_H
#define GL_MATERIAL_H

#include "defines.h"
#include "core/assets/types/model.h"
#include "core/assets/types/material.h"

#include <glad/glad.h>
#include <vector>

namespace ic
{

class Shader;

// ---------------------------------------------------------------------------
// GLTexture -> one OpenGL texture object (owns the GL handle)
//
// Ownership: GLModel::m_textures[]
// Indexed by Model::images() index, NOT the GLTF texture-list index.
// ---------------------------------------------------------------------------

struct GLTexture
{
        GLuint textureHandle = 0;

        void Upload(const Image &img);
        void ApplySampler(const Sampler *sampler) const;  // const: modifies GL state only
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

        // Texture slots: image indices into GLModel::m_textures[]
        int baseColorIdx     = -1;
        int metallicRoughIdx = -1;
        int normalIdx        = -1;
        int occlusionIdx     = -1;
        int emissiveIdx      = -1;

        // Sampler indices into Model::samplers()
        Index baseColorSampler     = INVALID_INDEX;
        Index metallicRoughSampler = INVALID_INDEX;
        Index normalSampler        = INVALID_INDEX;
        Index occlusionSampler     = INVALID_INDEX;
        Index emissiveSampler      = INVALID_INDEX;

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
        void Bind(Shader *shader, const std::vector<GLTexture> &textures, const std::vector<Sampler> &samplers) const;

        /**
         * Apply blend and cull-face GL state for this material.
         * The calling pass is responsible for restoring state afterwards.
         */
        void ApplyRenderState() const;
};

}  // namespace ic

#endif  // GL_MATERIAL_H