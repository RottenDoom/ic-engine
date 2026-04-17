#ifndef MATERIAL_H
#define MATERIAL_H

#include "defines.h"
#include "core/assets/types/asset_base.h"
#include "core/math.h"

#include <optional>
#include <string>

/**
 * material.h -> Runtime material types.
 *
 * Design rules:
 *   - All indices here are RESOLVED at build time by ModelBuilder.
 *     There are no GLTF accessor or texture-list references left.
 *   - TextureRef is the single source of truth for "a texture slot".
 *     NormalTextureRef / OcclusionTextureRef compose it, not inherit it.
 *   - KHR extension material data is optional<> so base PBR materials
 *     pay zero cost for extensions they don't use.
 *   - No raw pointer getDefaultMaterial() -> use the static factory.
 */

// ---------------------------------------------------------------------------
// TextureRef -> resolved image + sampler index pair
// texCoord selects which UV set (TEXCOORD_0, TEXCOORD_1, ...)
// ---------------------------------------------------------------------------

struct TextureRef
{
        Index image    = INVALID_INDEX;  // into Model::images
        Index sampler  = INVALID_INDEX;  // into Model::samplers
        Index texCoord = 0;              // UV set index

        bool isValid() const { return image != INVALID_INDEX; }
};

// ---------------------------------------------------------------------------
// Texture slot variants that carry extra per-slot parameters
// Use composition, NOT inheritance -> avoids the double-indirection trap
// ---------------------------------------------------------------------------

struct NormalTextureRef
{
        TextureRef ref;
        float      scale = 1.0f;

        bool isValid() const { return ref.isValid(); }
};

struct OcclusionTextureRef
{
        TextureRef ref;
        float      strength = 1.0f;

        bool isValid() const { return ref.isValid(); }
};

// ---------------------------------------------------------------------------
// Core PBR -> metallic/roughness workflow (GLTF 2.0 core)
// ---------------------------------------------------------------------------

struct PBRMetallicRoughness
{
        glm::vec4  baseColorFactor = glm::vec4(1.0f);
        TextureRef baseColorTexture;

        float      metallicFactor  = 1.0f;
        float      roughnessFactor = 1.0f;
        TextureRef metallicRoughnessTexture;
};

// ---------------------------------------------------------------------------
// KHR extension material data
// Each is wrapped in std::optional -> only allocated when the material uses it.
// ---------------------------------------------------------------------------

/** KHR_materials_anisotropy */
struct AnisotropyData
{
        float      anisotropyStrength = 0.0f;
        float      anisotropyRotation = 0.0f;
        TextureRef anisotropyTexture;
};

/** KHR_materials_specular */
struct SpecularData
{
        float      specularFactor = 1.0f;
        TextureRef specularTexture;
        glm::vec3  specularColorFactor = glm::vec3(1.0f);
        TextureRef specularColorTexture;
};

/** KHR_materials_iridescence */
struct IridescenceData
{
        float      iridescenceFactor = 0.0f;
        TextureRef iridescenceTexture;
        float      iridescenceIor          = 1.3f;
        float      iridescenceThicknessMin = 100.0f;
        float      iridescenceThicknessMax = 400.0f;
        TextureRef iridescenceThicknessTexture;
};

/** KHR_materials_diffuse_transmission */
struct DiffuseTransmissionData
{
        float      diffuseTransmissionFactor = 0.0f;
        TextureRef diffuseTransmissionTexture;
        glm::vec3  diffuseTransmissionColorFactor = glm::vec3(1.0f);
        TextureRef diffuseTransmissionColorTexture;
};

/** KHR_materials_transmission (volume/glass) */
struct TransmissionData
{
        float      transmissionFactor = 0.0f;
        TextureRef transmissionTexture;
};

// ---------------------------------------------------------------------------
// Material -> runtime PBR material
//
// Only holds what the renderer needs per draw call.
// Import-time data (GLTF alpha mode enums, extension strings) was resolved
// by ModelBuilder and is not stored here.
// ---------------------------------------------------------------------------

struct Material
{
        std::string name;

        // --- Core PBR ---
        PBRMetallicRoughness pbr;

        NormalTextureRef    normalTexture;
        OcclusionTextureRef occlusionTexture;
        TextureRef          emissiveTexture;
        glm::vec3           emissiveFactor   = glm::vec3(0.0f);
        float               emissiveStrength = 1.0f;

        // --- Alpha ---
        enum class AlphaMode : uint8_t
        {
                Opaque = 0,
                Mask,
                Blend
        } alphaMode = AlphaMode::Opaque;

        float alphaCutoff = 0.5f;

        // --- Surface flags ---
        bool doubleSided = false;
        bool unlit       = false;  // KHR_materials_unlit

        // --- Optical properties ---
        float ior        = 1.5f;  // KHR_materials_ior
        float dispersion = 0.0f;  // KHR_materials_dispersion

        // --- KHR extensions (zero-cost when unused) ---
        std::optional<AnisotropyData>          anisotropy;
        std::optional<SpecularData>            specular;
        std::optional<IridescenceData>         iridescence;
        std::optional<DiffuseTransmissionData> diffuseTransmission;
        std::optional<TransmissionData>        transmission;

        // --- Factory ---
        /** Returns a default opaque white PBR material with no textures. */
        static Material makeDefault();

        /** Returns true if any extension slot is populated. */
        bool hasExtensions() const
        {
                return anisotropy.has_value() || specular.has_value() || iridescence.has_value() ||
                       diffuseTransmission.has_value() || transmission.has_value();
        }
};

inline Material Material::makeDefault()
{
        Material m;
        m.name                = "default";
        m.pbr.baseColorFactor = glm::vec4(1.0f);
        m.pbr.metallicFactor  = 0.0f;
        m.pbr.roughnessFactor = 1.0f;
        m.alphaMode           = AlphaMode::Opaque;
        return m;
}

#endif