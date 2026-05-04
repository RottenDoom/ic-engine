#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include "defines.h"
#include "core/math.h"
#include "core/assets/types/asset_base.h"

#include <string>
#include <vector>

/**
 * model_data.h
 *
 * All structs in this file are IMPORT-ONLY intermediates.
 * They are produced by IModelLoader implementations (e.g. GLTFLoader)
 * and consumed by ModelBuilder to produce a runtime Model.
 *
 * Nothing in this file should be included by the runtime Model or
 * any renderer-side code. These types exist solely to decouple
 * the file format from the engine asset.
 */

namespace ic
{

// ---------------------------------------------------------------------------
// Low-level GLTF buffer intermediates
// These are consumed during geometry processing and discarded afterward.
// ---------------------------------------------------------------------------

/** Raw byte buffer sourced from a .bin file, GLB chunk, or embedded base64 */
struct Buffer
{
        std::vector<uint8_t> data;
};

/** Describes a slice of a Buffer -> maps to a GLTF bufferView */
struct BufferView
{
        Index       bufferIndex = INVALID_INDEX;
        size_t      byteOffset  = 0;
        size_t      byteLength  = 0;
        size_t      byteStride  = 0;  // 0 means tightly packed
        std::string name;
};

/**
 * Typed view into a BufferView.
 * min/max are stored as double to handle both float64 and int64 GLTF bounds.
 */
struct Accessor
{
        Index  bufferView = INVALID_INDEX;
        size_t offset     = 0;
        size_t count      = 0;

        enum class Type : uint8_t
        {
                SCALAR,
                VEC2,
                VEC3,
                VEC4,
                MAT4,
                UNKNOWN
        } type = Type::UNKNOWN;

        enum class ComponentType : uint8_t
        {
                Byte,
                UByte,
                Short,
                UShort,
                Int,
                UInt,
                Float,
                Double
        } componentType = ComponentType::UByte;

        std::vector<double> min;  // size matches component count of Type
        std::vector<double> max;
        bool                normalized = false;
};

// ---------------------------------------------------------------------------
// Per-vertex data -> used only during geometry extraction in GLTFLoader
// ---------------------------------------------------------------------------

struct Vertex
{
        vec3  pos     = {};
        vec3  normal  = {};
        vec2  uv0     = {};
        vec2  uv1     = {};
        vec2  uv2     = {};
        vec4  color   = {};
        vec4  tangent = {};
        uvec4 joint0  = {};
        vec4  weight0 = {};
};

// ---------------------------------------------------------------------------
// Accessor index cache -> one per MeshPrimitive, used during processMeshGeometry
// Cleared after geometry extraction is complete.
// ---------------------------------------------------------------------------

struct TempPrimitiveData
{
        Index positionAccessor  = INVALID_INDEX;
        Index normalAccessor    = INVALID_INDEX;
        Index tangentAccessor   = INVALID_INDEX;
        Index colorAccessor     = INVALID_INDEX;
        Index texCoord0Accessor = INVALID_INDEX;
        Index texCoord1Accessor = INVALID_INDEX;
        Index texCoord2Accessor = INVALID_INDEX;  // uv2 support
        Index jointsAccessor    = INVALID_INDEX;
        Index weightsAccessor   = INVALID_INDEX;
        Index indicesAccessor   = INVALID_INDEX;
};

// ---------------------------------------------------------------------------
// MeshImportData
// Mirrors the runtime Mesh/MeshPrimitive but still contains CPU vertex data.
// ModelBuilder will move vertex/index vectors into runtime primitives.
// ---------------------------------------------------------------------------

struct MeshPrimitiveImportData
{
        enum class Mode : uint8_t
        {
                POINTS        = 0,
                LINES         = 1,
                LINELOOP      = 2,
                LINESTRIP     = 3,
                TRIANGLES     = 4,
                TRIANGLESTRIP = 5,
                TRIANGLEFAN   = 6,
        } mode = Mode::TRIANGLES;

        Index materialIndex = INVALID_INDEX;

        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;

        std::vector<float> morphWeights;

        // AABB computed from position accessor min/max during extraction
        // avoids a second pass over vertices
        vec3 aabbMin = vec3(0.0f);
        vec3 aabbMax = vec3(0.0f);
};

struct MeshImportData
{
        std::string                          name;
        std::vector<MeshPrimitiveImportData> primitives;

        // Mesh-level AABB, merged from all primitive AABBs by ModelBuilder
        vec3 aabbMin = vec3(0.0f);
        vec3 aabbMax = vec3(0.0f);
};

// ---------------------------------------------------------------------------
// ImageImportData
// Raw decoded pixel data from stb_image. Already RGBA8 at this point.
// ---------------------------------------------------------------------------

struct ImageImportData
{
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;  // original channel count before forced RGBA
        bool     srgb     = false;

        std::string name;
        std::string uri;  // original source path, empty for embedded images

        // Always RGBA8 after stb_image decode (forced to 4 channels)
        std::vector<uint8_t> pixels;
};

// ---------------------------------------------------------------------------
// SamplerImportData
// Wraps GLTF texture filter and wrap mode enums.
// Values match OpenGL constants intentionally for easy mapping.
// ---------------------------------------------------------------------------

struct SamplerImportData
{
        enum class Filter : uint16_t
        {
                Nearest              = 9728,
                Linear               = 9729,
                NearestMipMapNearest = 9984,
                LinearMipMapNearest  = 9985,
                NearestMipMapLinear  = 9986,
                LinearMipMapLinear   = 9987,
                NoFilter             = 0
        };

        enum class Wrap : uint16_t
        {
                ClampToEdge    = 33071,
                MirroredRepeat = 33648,
                Repeat         = 10497,
                NoWrap         = 0
        };

        Filter magFilter = Filter::NoFilter;
        Filter minFilter = Filter::NoFilter;
        Wrap   wrapS     = Wrap::NoWrap;
        Wrap   wrapT     = Wrap::NoWrap;
};

// ---------------------------------------------------------------------------
// TextureImportData
// Thin index pair -> just links an image to a sampler.
// ---------------------------------------------------------------------------

struct TextureImportData
{
        Index image   = INVALID_INDEX;
        Index sampler = INVALID_INDEX;
};

// ---------------------------------------------------------------------------
// MaterialImportData
// Full PBR material as loaded from GLTF. Mirrors the runtime Material
// but is kept separate so Material.h has no GLTF dependency.
// ---------------------------------------------------------------------------

struct TextureRefImportData
{
        Index idx      = INVALID_INDEX;
        Index texCoord = 0;
};

struct NormalTextureImportData
{
        TextureRefImportData ref;
        float                scale = 1.0f;
};

struct OcclusionTextureImportData
{
        TextureRefImportData ref;
        float                strength = 1.0f;
};

struct PBRMetallicRoughnessImportData
{
        vec4  baseColorFactor = vec4(1.0f);
        float metallicFactor  = 1.0f;
        float roughnessFactor = 1.0f;

        TextureRefImportData baseColorTexture;
        TextureRefImportData metallicRoughnessTexture;
};

struct MaterialImportData
{
        std::string name;

        PBRMetallicRoughnessImportData pbr;

        NormalTextureImportData    normalTexture;
        OcclusionTextureImportData occlusionTexture;
        TextureRefImportData       emissiveTexture;

        vec3  emissiveFactor = vec3(0.0f);
        float alphaCutoff    = 0.5f;

        enum class AlphaMode : uint8_t
        {
                Opaque = 0,
                Mask,
                Blend
        } alphaMode = AlphaMode::Opaque;

        bool doubleSided = false;
};

// ---------------------------------------------------------------------------
// NodeImportData
// Scene graph node. Stores both TRS components and the computed local matrix.
// worldTransform is NOT computed here -> that is ModelBuilder's job.
// ---------------------------------------------------------------------------

struct NodeImportData
{
        std::string name;

        vec3 translation    = vec3(0.0f);
        quat rotation       = quat(1.0f, 0.0f, 0.0f, 0.0f);
        vec3 scale          = vec3(1.0f);
        mat4 localTransform = mat4(1.0f);

        Index meshIndex   = INVALID_INDEX;
        Index skinIndex   = INVALID_INDEX;
        Index cameraIndex = INVALID_INDEX;
        Index lightIndex  = INVALID_INDEX;

        std::vector<Index> children;
        Index              parent = INVALID_INDEX;  // filled by ModelBuilder during hierarchy pass
};

// ---------------------------------------------------------------------------
// CameraImportData
// ---------------------------------------------------------------------------

struct CameraImportData
{
        enum class Type : uint8_t
        {
                Perspective,
                Orthographic
        };

        struct Perspective
        {
                float aspectRatio = 0.0f;
                float yfov        = 0.0f;
                float zfar        = 0.0f;
                float znear       = 0.0f;
        };

        struct Orthographic
        {
                float xmag  = 0.0f;
                float ymag  = 0.0f;
                float zfar  = 0.0f;
                float znear = 0.0f;
        };

        std::string  name;
        Type         type = Type::Perspective;
        Perspective  perspective;
        Orthographic orthographic;
};

// ---------------------------------------------------------------------------
// SkinImportData
// Joint indices reference the nodes array. inverseBindMatrices are already
// decoded from the accessor by the loader -> no Accessor index stored here.
// ---------------------------------------------------------------------------

struct SkinImportData
{
        std::string        name;
        std::vector<Index> jointIndices;
        std::vector<mat4>  inverseBindMatrices;
        Index              skeletonRootIndex = INVALID_INDEX;
};

// ---------------------------------------------------------------------------
// AnimationImportData
// Keyframe data is fully decoded from accessors into plain float vectors.
// The loader resolves all accessor indirection here -> no accessor indices
// escape into AnimationImportData.
// ---------------------------------------------------------------------------

struct AnimationSamplerImportData
{
        enum class Interpolation : uint8_t
        {
                Linear,
                Step,
                CubicSpline
        };

        std::vector<float> inputTimes;    // keyframe timestamps
        std::vector<vec4>  outputValues;  // vec4 covers translation(vec3), rotation(quat), scale(vec3), weights
        Interpolation      interpolation = Interpolation::Linear;
};

struct AnimationChannelImportData
{
        enum class Path : uint8_t
        {
                Translation,
                Rotation,
                Scale,
                Weights
        };

        Index samplerIndex    = INVALID_INDEX;
        Index targetNodeIndex = INVALID_INDEX;
        Path  targetPath      = Path::Translation;
};

struct AnimationImportData
{
        std::string                             name;
        std::vector<AnimationSamplerImportData> samplers;
        std::vector<AnimationChannelImportData> channels;
        float                                   duration = 0.0f;  // max inputTime across all samplers
};

// ---------------------------------------------------------------------------
// SceneImportData
// ---------------------------------------------------------------------------

struct SceneImportData
{
        std::string        name;
        std::vector<Index> rootNodes;
};

// ---------------------------------------------------------------------------
// ModelImportData -> top-level container produced by IModelLoader
//
// Lifecycle:
//   1. GLTFLoader fills this entirely (including buffer intermediates)
//   2. ModelBuilder consumes it via std::move to produce a runtime Model
//   3. This struct is destroyed -> nothing escapes into the runtime
// ---------------------------------------------------------------------------

struct ModelImportData
{
        // Fully-processed import data (runtime-compatible shapes)
        std::vector<MeshImportData>      meshes;
        std::vector<MaterialImportData>  materials;
        std::vector<ImageImportData>     images;
        std::vector<SamplerImportData>   samplers;
        std::vector<TextureImportData>   textures;
        std::vector<NodeImportData>      nodes;
        std::vector<CameraImportData>    cameras;
        std::vector<SkinImportData>      skins;
        std::vector<AnimationImportData> animations;
        std::vector<SceneImportData>     scenes;

        Index       defaultScene = INVALID_INDEX;
        std::string sourceFormat;  // "gltf", "obj", "fbx" -> informational only

        // GLTF buffer intermediates -> only valid during loading, cleared after
        // processMeshGeometry() completes. Do NOT access these after ModelImportData
        // is handed to ModelBuilder.
        std::vector<Buffer>     buffers;
        std::vector<BufferView> bufferViews;
        std::vector<Accessor>   accessors;

        /** Releases buffer intermediates after geometry extraction.
         *  Call this at the end of GLTFLoader::loadGLTF, before returning. */
        void freeIntermediates()
        {
                buffers.clear();
                buffers.shrink_to_fit();
                bufferViews.clear();
                bufferViews.shrink_to_fit();
                accessors.clear();
                accessors.shrink_to_fit();
        }
};

}  // namespace ic

#endif  // MODEL_DATA_H