#ifndef ASSET_VERSIONS_H
#define ASSET_VERSIONS_H
#include "defines.h"
#include "types/asset_base.h"

// Asset Types
// If order is changed here, ALL cache files must be rebuilt.

inline const char *const g_assetNames[] = {
    "Image",       // ASSET_TYPE_GFX_IMAGE
    "Material",    // ASSET_TYPE_MATERIAL
    "Script",      // ASSET_TYPE_SCRIPT
    "Model",       // ASSET_TYPE_MODEL
    "Shader",      // ASSET_TYPE_SHADER
    "Pipeline",    // ASSET_TYPE_PIPELINE
    "Font",        // ASSET_TYPE_FONT
    "Textureset",  // ASSET_TYPE_TEXTURESET
};

// =============================================================================
// Per-asset version history
//
// HOW TO BUMP:
//   1. Add a new line at the bottom of the relevant block.
//   2. Write what changed in the comment — future you will thank you.
//   3. That's it. IC_ASSET_VERSION updates automatically.
//
// RULES:
//   - Never delete or reorder lines. Only append.
//   - One line = one breaking change to the serialized format.
//   - Non-breaking changes (adding optional data, no layout change) do NOT
//     need a bump — only bump when old readers would misread the new format.
// =============================================================================

#define IC_GLOBAL_ASSET_VERSION 0x4943 // 18755 / IC
constexpr uint32_t IC_ASSET_MAGIC = 0x49434D44;  // 'ICMD'

namespace AssetVersion
{
// -------------------------------------------------------------------------
// ASSET_TYPE_GFX_IMAGE
// -------------------------------------------------------------------------
enum GfxImage : int32_t
{
        GfxImage_Initial = 1,  // baseline
        // GfxImage_MipData           = 2,   // added mip level storage
        // GfxImage_SRGBFlag          = 3,   // added sRGB flag
        // GfxImage_ArrayLayers       = 4,   // cubemap / array support
        // GfxImage_CompressionType   = 5,   // BC1-BC7 type stored
        // GfxImage_ChannelSwizzle    = 6,   // swizzle mask added
        // GfxImage_StreamingLOD      = 7,   // streaming mip offset
        // GfxImage_EmbeddedSampler   = 8,   // sampler baked into image
        // GfxImage_NameSerialization = 9,
        // GfxImage_TexturesetBump    = 10,  // textureset layout changed — bump here not textureset

        GfxImage_CURRENT = GfxImage_Initial,
};

// -------------------------------------------------------------------------
// ASSET_TYPE_MATERIAL
// -------------------------------------------------------------------------
enum Material : int32_t
{
        Material_Initial = 1,
        // Material_AlphaMode         = 2,
        // Material_DoubleSided       = 3,
        // Material_EmissiveFactor    = 4,
        // Material_TextureSlotCount  = 5,
        // Material_PBRMetalRoughness = 6,
        // Material_KHR_Transmission  = 7,
        // Material_KHR_Iridescence   = 8,
        // Material_KHR_Anisotropy    = 9,
        // Material_KHR_Specular      = 10,
        // Material_NameSerialization = 11,

        Material_CURRENT = Material_Initial,
};

// -------------------------------------------------------------------------
// ASSET_TYPE_SCRIPT
// -------------------------------------------------------------------------
enum Script : int32_t
{
        Script_Initial           = 1,
        Script_NameSerialization = 2,

        Script_CURRENT = Script_NameSerialization,
};

// -------------------------------------------------------------------------
// ASSET_TYPE_MODEL
// -------------------------------------------------------------------------
enum Model : int32_t
{
        Model_Initial = 1,
        // Model_AABB                 = 2,
        // Model_SkinData             = 3,
        // Model_AnimationData        = 4,
        // Model_InterlevedVertexData = 5,
        // Model_AttributeFlags       = 6,
        // Model_NodeHierarchy        = 7,
        // Model_MaterialRefs         = 8,
        // Model_MultipleUVSets       = 9,
        // Model_WorldTransforms      = 10,
        // Model_MorphTargets         = 11,
        // Model_SceneList            = 12,
        // Model_CameraNodes          = 13,
        // Model_SamplerData          = 14,
        // Model_ImageSRGB            = 15,
        // Model_NameSerialization    = 16,
        // Model_IndexType32          = 17,
        // Model_VertexStrideExplicit = 18,
        // Model_PackedUVs            = 19,
        // Model_CacheHeader          = 20,

        Model_CURRENT = Model_Initial,
};

// -------------------------------------------------------------------------
// ASSET_TYPE_SHADER
// -------------------------------------------------------------------------
enum Shader : int32_t
{
        Shader_Initial = 1,
        // Shader_DefineSupport    = 2,
        // Shader_ExtensionFixed   = 3,
        // Shader_DefineUsageFixed = 4,
        // Shader_ReflectionData   = 5,
        // Shader_SpecConstants    = 6,

        Shader_CURRENT = Shader_Initial,
};

// -------------------------------------------------------------------------
// ASSET_TYPE_PIPELINE
// -------------------------------------------------------------------------
enum Pipeline : int32_t
{
        Pipeline_Initial = 1,
        // Pipeline_BlendState       = 2,
        // Pipeline_DepthStencil     = 3,
        // Pipeline_Multisampling    = 4,
        // Pipeline_ExtensionFixed   = 5,
        // Pipeline_DefineUsageFixed = 6,

        Pipeline_CURRENT = Pipeline_Initial,
};

// -------------------------------------------------------------------------
// ASSET_TYPE_FONT
// -------------------------------------------------------------------------
enum Font : int32_t
{
        Font_Initial = 1,
        // Font_KerningTable     = 2,
        // Font_SDFParams        = 3,
        // Font_EdgeColoringMode = 4,
        // Font_UnicodeRange     = 5,

        Font_CURRENT = Font_Initial,
};

}  // namespace AssetVersion

// Cache version — derived automatically, never set by hand.
//
// Computed as: base + sum of all current per-asset versions.
// Any bump to any asset type increments this, invalidating all caches.
// That's intentional — it's the simplest correct behavior.
//
// TODO: store these versions in the cache for fast file loads

namespace detail
{
constexpr int32_t g_assetVersions[] = {
    AssetVersion::GfxImage_CURRENT,  // ASSET_TYPE_GFX_IMAGE
    AssetVersion::Material_CURRENT,  // ASSET_TYPE_MATERIAL
    AssetVersion::Script_CURRENT,    // ASSET_TYPE_SCRIPT
    AssetVersion::Model_CURRENT,     // ASSET_TYPE_MODEL
    AssetVersion::Shader_CURRENT,    // ASSET_TYPE_SHADER
    AssetVersion::Pipeline_CURRENT,  // ASSET_TYPE_PIPELINE
    AssetVersion::Font_CURRENT,      // ASSET_TYPE_FONT
    0,                               // ASSET_TYPE_TEXTURESET — no .ffi, no version
};

constexpr int32_t sumVersions()
{
        int32_t sum = 0;
        for (int32_t v : g_assetVersions)
                sum += v;
        return sum;
}
}  // namespace detail

constexpr uint32_t IC_CACHE_VERSION = IC_GLOBAL_ASSET_VERSION + detail::sumVersions();

//   if (header.version != assetCurrentVersion(ASSET_TYPE_MODEL)) { rebuild; }
constexpr int32_t assetCurrentVersion(AssetType type)
{
        return (type < ASSET_TYPE_COUNT) ? detail::g_assetVersions[type] : 0;
}

#endif  // ASSET_VERSIONS_H