#ifndef MATERIAL_H
#define MATERIAL_H

#include "defines.h"
#include "core/assets/types/asset_base.h"
#include "core/math.h"
#include "core/uuid.h"

#include <optional>
#include <string>

namespace ic
{

/** TextureHandle and MaterialHandle resolves to assetmanager asset handles */

using MaterialHandle = uint64_t;
using TextureHandle  = uint64_t;

struct NormalTexture
{
        TextureHandle ref;
        float         scale = 1.0f;
};

struct OcclusionTexture
{
        TextureHandle ref;
        float         strength = 1.0f;
};

struct PBRMetallicRoughness
{
        glm::vec4     baseColorFactor = glm::vec4(1.0f);
        TextureHandle baseColorTexture;

        float         metallicFactor  = 1.0f;
        float         roughnessFactor = 1.0f;
        TextureHandle metallicRoughnessTexture;
};

struct EmissiveTexture
{
        TextureHandle emissiveTexture;
        glm::vec3     emissiveFactor   = glm::vec3(0.0f);
        float         emissiveStrength = 1.0f;
};

/**  ============= KHR extension material data ========== */

/** KHR_materials_anisotropy */
struct AnisotropyData
{
        float         anisotropyStrength = 0.0f;
        float         anisotropyRotation = 0.0f;
        TextureHandle anisotropyTexture;
};

/** KHR_materials_specular */
struct SpecularData
{
        float         specularFactor = 1.0f;
        TextureHandle specularTexture;
        glm::vec3     specularColorFactor = glm::vec3(1.0f);
        TextureHandle specularColorTexture;
};

/** KHR_materials_iridescence */
struct IridescenceData
{
        float         iridescenceFactor = 0.0f;
        TextureHandle iridescenceTexture;
        float         iridescenceIor          = 1.3f;
        float         iridescenceThicknessMin = 100.0f;
        float         iridescenceThicknessMax = 400.0f;
        TextureHandle iridescenceThicknessTexture;
};

/** KHR_materials_diffuse_transmission */
struct DiffuseTransmissionData
{
        float         diffuseTransmissionFactor = 0.0f;
        TextureHandle diffuseTransmissionTexture;
        glm::vec3     diffuseTransmissionColorFactor = glm::vec3(1.0f);
        TextureHandle diffuseTransmissionColorTexture;
};

/** KHR_materials_transmission (volume/glass) */
struct TransmissionData
{
        float         transmissionFactor = 0.0f;
        TextureHandle transmissionTexture;
};

struct Material
{
        string name;

        // --- Core PBR ---
        PBRMetallicRoughness pbr;

        NormalTexture    normalTexture;
        OcclusionTexture occlusionTexture;
        EmissiveTexture  emissiveTexture;

        // --- Alpha ---
        enum class AlphaMode : uint8_t
        {
                Opaque = 0,
                Mask,
                Blend
        } alphaMode       = AlphaMode::Opaque;
        float alphaCutoff = 0.5f;

        // Surface lighting flags
        bool doubleSided = false;
        bool unlit       = false;  // KHR_materials_unlit

        // Optical properties
        float ior        = 1.5f;  // KHR_materials_ior
        float dispersion = 0.0f;  // KHR_materials_dispersion

        // KHR Extensions
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

/** Material Asset used for referencing from mesh primitives into asset manager */
class MaterialAsset : public IAsset
{
public:
        ASSET_CLASS_TYPE(ASSET_TYPE_MATERIAL);
        MaterialAsset(MaterialHandle id) : IAsset(id) {}

        // TODO: Loading and serializing materials from .mtl files etc.
        bool Load(const char *path) override { return false; }
        bool SerializedLoad(ic::Serializer *serializer) override { return false; }
        bool SerializedSave(ic::Serializer *serializer) const override { return false; }
        bool Release() override { return false; }

        void      SetMaterial(Material &material) { m_material = material; }
        Material &GetMaterial() { return m_material; }

private:
        Material m_material;
        string   name;
};

}  // namespace ic

#endif  // MATERIAL_H