#ifndef MODEL_H
#define MODEL_H

#include "defines.h"
#include "core/math.h"
#include "core/filesystem.h"

#include "core/assets/types/asset_base.h"

#include <string>  // [TODO] Make string class using std::vector or a custom dynamic array
#include <vector>

// #if USING(GPU_DATA)
// #include "renderer/graphics_api/buffer.hpp"
// #include "renderer/graphics_api/texture.hpp"
// #endif

using Index                   = uint32_t;
constexpr Index INVALID_INDEX = ~0u;

class ic::GLTFLoader;

struct TextureInfo;
struct Texture;
struct Material;

struct Vertex
{
        vec3 pos;
        vec3 normal;
        vec2 uv0;
        vec2 uv1;
        vec2 uv2;
        vec4 color;
        vec4 tangent;
        uvec4 joint0;
        vec4 weight0;
};

/** GPU data with the actual buffer*/
struct Buffer
{
        std::vector<uint8_t> data;  // char data from the the uri or glb
};

struct BufferView
{
        Index bufferIndex = INVALID_INDEX;
        size_t byteOffset = 0;
        size_t byteLength = 0;
        size_t byteStride = 0;

        std::string name;

        /* TODO OpenGL specific Target*/
};

struct Accessor
{
        Index bufferView = INVALID_INDEX;
        size_t offset    = 0;
        size_t count     = 0;

        enum class Type
        {
                SCALAR,
                VEC2,
                VEC3,
                VEC4,
                MAT4,
                UNKNOWN
        } type;

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
        };

        /** OpenGL specific */
        ComponentType componentType = ComponentType::UByte;

        std::vector<double> min;
        std::vector<double> max;
        bool normalized = false;
        /** TODO: Sparse accessor handling */
};

struct MeshPrimitive
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
        };

        // std::string name;
        Mode mode           = Mode::TRIANGLES;
        Index materialIndex = INVALID_INDEX;

        // CPU-side geometry data (serializable)
        // #if USING(GPU_DATA)
        //         // Empty on CPU when GPU data is present
        // #else
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        // #endif

        // #if USING(GPU_DATA)
        //         // GPU buffers (not serialized, recreated on load)
        //         Gfx::Buffer *vertexBuffer    = nullptr;
        //         Gfx::Buffer *indexBuffer     = nullptr;
        //         uint32_t numVertices         = 0;
        //         uint32_t numIndices          = 0;
        //         uint32_t bindlessBuffersSlot = 0;

        //         // Attribute info for rendering
        //         bool hasNormals   = false;
        //         bool hasTangents  = false;
        //         bool hasTexCoord0 = false;
        //         bool hasTexCoord1 = false;
        //         bool hasColors    = false;
        //         bool hasSkinning  = false;
        // #endif

        // Morph targets (if needed)
        std::vector<float> morphWeights;
};

struct Mesh
{
        std::string name;
        std::vector<MeshPrimitive> primitives;

        // Make AABB struct
        glm::vec3 aabbMin = glm::vec3(0.0f);
        glm::vec3 aabbMax = glm::vec3(0.0f);
};

struct Node
{
        string name;

        glm::vec3 translation    = glm::vec3(0.0f);
        glm::quat rotation       = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale          = glm::vec3(1.0f);
        glm::mat4 localTransform = glm::mat4(1.0f);
        glm::mat4 worldTransform = glm::mat4(1.0f);  // Computed from hierarchy

        Index meshIndex          = INVALID_INDEX;
        Index skinIndex          = INVALID_INDEX;
        Index cameraIndex        = INVALID_INDEX;
        Index lightIndex         = INVALID_INDEX;

        std::vector<Index> children;
        Index parent = INVALID_INDEX;
};

struct AssetCamera
{
        enum class Type
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

        string name;
        Type type;
        Perspective perspective;
        Orthographic orthographic;
};

struct Skin
{
        std::string name;
        std::vector<Index> jointIndices;             // Indices into nodes array
        std::vector<glm::mat4> inverseBindMatrices;  // Actual matrices, not accessor reference
        Index skeletonRootIndex = INVALID_INDEX;
};

struct Animation
{
        enum class Path
        {
                TRANSLATION,
                ROTATION,
                SCALE,
                WEIGHTS
        };

        enum class Interpolation
        {
                LINEAR,
                STEP,
                CUBICSPLINE
        };

        struct Sampler
        {
                std::vector<float> inputTimes;        // Actual keyframe times
                std::vector<glm::vec4> outputValues;  // Actual keyframe values (vec4 to handle all types)
                Interpolation interpolation = Interpolation::LINEAR;
        };

        struct Channel
        {
                Index samplerIndex    = INVALID_INDEX;
                Index targetNodeIndex = INVALID_INDEX;
                Path targetPath;
        };

        string name;
        std::vector<Sampler> samplers;
        std::vector<Channel> channels;
        float duration = 0.0f;  // Computed from max input time
};

struct Sampler
{
        enum class Filter : std::uint16_t
        {
                Nearest              = 9728,  // GL_NEAREST
                Linear               = 9729,  // GL_LINEAR
                NearestMipMapNearest = 9984,  // GL_NEAREST_MIPMAP_NEAREST
                LinearMipMapNearest  = 9985,  // GL_LINEAR_MIPMAP_NEAREST
                NearestMipMapLinear  = 9986,  // GL_NEAREST_MIPMAP_LINEAR
                LinearMipMapLinear   = 9987,  // GL_LINEAR_MIPMAP_LINEAR
                NoFilter             = 0
        };

        enum class Wrap : std::uint16_t
        {
                ClampToEdge    = 33071,
                MirroredRepeat = 33648,
                Repeat         = 10497,
                NoWrap         = 0
        };

        Filter magFilter = Filter::NoFilter;  // GL_TEXTURE_MAG_FILTER
        Filter minFilter = Filter::NoFilter;  // GL_TEXTURE_MIN_FILTER
        Wrap wrapS       = Wrap::NoWrap;
        Wrap wrapT       = Wrap::NoWrap;
};

struct ImageData
{
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;
        bool srgb         = false;

        // #if USING(GPU_DATA)
        //         Gfx::Texture *gpuTexture = nullptr;
        // #else
        std::vector<uint8_t> pixels;
        // #endif

        // #if USING(ASSET_NAMES)
        string name;
        string uri;
        // #endif
};

struct Scene
{
        std::string name;
        std::vector<Index> rootNodes;
};

class Model : public IAsset
{
        // NOT SURE ABOUT THE FRIEND CLASSES I THINK ONE SINGLE FRIEND FUNCTION SHOULD SUFFICE
        friend class ic::GLTFLoader;

public:
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<ImageData> images;
        std::vector<Sampler> samplers;
        std::vector<Texture> textures;

        std::vector<Node> nodes;
        std::vector<AssetCamera> cameras;
        std::vector<Skin> skins;
        std::vector<Animation> animations;
        std::vector<Scene> scenes;

        Index defaultScene = INVALID_INDEX;

        // TODO: SOMEDAY
        std::vector<std::string> extensionsUsed;
        std::vector<std::string> extensionsRequired;

        ASSET_CLASS_TYPE(ASSET_TYPE_MODEL)

        bool Load(const char *filepath) override;
        bool CachedLoad(ic::Serializer *serializer) override;
        bool CachedSave(ic::Serializer *serializer) const override;
        void Free() override;

        void FreeCPU();
        void FreeGPU();

        // Editor-friendly queries
        size_t GetMeshCount() const { return meshes.size(); }
        size_t GetNodeCount() const { return nodes.size(); }
        size_t GetAnimationCount() const { return animations.size(); }

        // const Mesh *GetMesh(Index index) const;
        // Mesh *GetMesh(Index index);
        // const Node *GetNode(Index index) const;
        // Node *GetNode(Index index);
        // const Material *GetMaterial(Index index) const;
        // Material *GetMaterial(Index index);

        // // Hierarchy queries
        // void GetRootNodes(std::vector<Index> &outRootNodes) const;
        // void GetNodeChildren(Index nodeIndex, std::vector<Index> &outChildren) const;
        // glm::mat4 GetNodeWorldTransform(Index nodeIndex) const;

        // // Animation control
        // void UpdateAnimation(Index animIndex, float time);
        // float GetAnimationDuration(Index animIndex) const;

        // // Rebuild operations (for editor modifications)
        // void RebuildNodeTransforms();
        // void RebuildBoundingBoxes();
        // void MarkGPUDirty();  // Flags that GPU data needs re-upload

private:
        // Raw GLTF loading data (temporary, freed after processing)
        std::vector<Buffer> buffers;
        std::vector<BufferView> bufferViews;
        std::vector<Accessor> accessors;

        // Internal state flags
        bool gpuDataDirty    = false;
        bool transformsDirty = false;

        // void FreeLoadingData();

        // // Processing pipeline (called during Load)
        // void LoadFromGLTF();
};

#endif