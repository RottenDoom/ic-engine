#pragma once
#include "defines.h"

#include <string>  // [TODO] Make string class using std::vector or a custom dynamic array
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ic
{

using Index                   = uint32_t;
constexpr Index INVALID_INDEX = ~0u;

/** GPU data with the actual buffer*/
struct Buffer
{
        std::vector<uint8_t> data;  // char data from the the uri or glb
};

struct BufferView
{
        Index buffer  = INVALID_INDEX;
        size_t offset = 0;
        size_t size   = 0;
        size_t stride = 0;

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

        /** OpenGL specific */
        GLenum componentType;

        std::vector<double> min;
        std::vector<double> max;
        bool normalized = false;
        /** TODO: Sparse accessor handling */
};

/* I am probably going to make a different implementation in another file */
struct Vertex
{
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::uvec4 joint0;
        glm::vec4 weight0;
        glm::vec4 color;
        glm::vec4 tangent;

        /* any other things*/
};

struct Sampler
{
        enum class Filter : std::uint16_t
        {
                Nearest              = 9728,
                Linear               = 9729,
                NearestMipMapNearest = 9984,
                LinearMipMapNearest  = 9985,
                NearestMipMapLinear  = 9986,
                LinearMipMapLinear   = 9987,
                NoFilter             = 0
        };

        enum class Wrap : std::uint16_t
        {
                ClampToEdge    = 33071,
                MirroredRepeat = 33648,
                Repeat         = 10497,
                NoWrap         = 0
        };

        Filter magFilter = Filter::NoFilter;
        Filter minFilter = Filter::NoFilter;
        Wrap wrapS       = Wrap::NoWrap;
        Wrap wrapT       = Wrap::NoWrap;
};

struct ImageData
{
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;

        // Raw decoded pixels (RGBA8, etc.)
        std::vector<uint8_t> pixels;

        // Optional metadata
        bool srgb = false;
};

struct Texture
{
        Index image   = INVALID_INDEX;
        Index sampler = INVALID_INDEX;
};

struct Material
{
        /* for gltf defualt model is metallic roughness model */

        enum AlphaMode : uint8_t
        {
                ALPHAMODE_OPAQUE,
                ALPHAMODE_MASK,
                ALPHAMODE_BLEND
        };

        AlphaMode alphaMode = ALPHAMODE_OPAQUE;
        float alphaCutoff   = 1.0f;

        /** Make a texture class instead of this bro */
        typedef struct
        {
                Index baseColorTextureIndex = INVALID_INDEX;
                Index baseColorTextureCoord = INVALID_INDEX;

                glm::vec4 baseColorFactor;  // RGBA

                Index metallicRoughnessTextureIndex = INVALID_INDEX;
                Index metallicRoughnessTextureCoord = INVALID_INDEX;

                float metallicFactor                = 1.0f;
                float roughnessFactor               = 1.0f;

        } PbrMetallicRoughness;

        typedef struct
        {
                float scale;
                Index index;
                uint8_t texCoord;
        } NormalTexture;

        typedef struct
        {
                float strength;
                Index index;
                uint8_t texCoord;
        } OcclusionTexture;

        typedef struct
        {
                Index index;
                uint8_t texCoord;
        } EmissiveTexture;

        PbrMetallicRoughness pbr;
        NormalTexture normal;
        OcclusionTexture occlusion;
        EmissiveTexture emissive;

        glm::vec3 emissiveFactor;  // RGB

        std::string name;
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

        Mode mode = Mode::TRIANGLES;

        // core attributes
        Index position = INVALID_INDEX;
        Index normal   = INVALID_INDEX;
        Index tangent  = INVALID_INDEX;
        Index color    = INVALID_INDEX;

        // UV Texcoords
        std::vector<Index> texcoords;

        // Skinning
        Index joints  = INVALID_INDEX;
        Index weights = INVALID_INDEX;

        // Indices & material
        Index indices  = INVALID_INDEX;
        Index material = INVALID_INDEX;

        // [TODO] targets
};

struct Mesh
{
        std::string name;
        std::vector<MeshPrimitive> meshPrimitives;
        std::vector<float> weights; /* somehow related to morph targets */
};

struct Node
{
        std::string name;

        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;

        glm::mat4 localTransform = glm::mat4(1.0f);

        Index mesh               = INVALID_INDEX;
        Index skin               = INVALID_INDEX;
        Index light              = INVALID_INDEX;

        Index camera             = INVALID_INDEX;

        std::vector<Index> children;
};

struct AssetCamera
{
        typedef struct
        {
                float aspectRatio;
                float yfov;
                float zfar;
                float znear;
        } PerspectiveCamera;

        typedef struct
        {
                float xmag;
                float ymag;
                float zfar;
                float znear;
        } OrthographicCamera;

        PerspectiveCamera pCamera;
        OrthographicCamera oCamera;
};

struct Skin
{
        std::vector<Index> joints;
        Index inverseBindMatrices = INVALID_INDEX;
};

struct Animation
{
        typedef enum
        {
                TRANSLATION,
                ROTATION,
                SCALE
        } AnimationPath;

        typedef enum
        {
                LINEAR,
                STEP,
                CUBICSPLINE
        } Interpolation;

        typedef struct
        {
                /** These refer to the times of the key frames of the animations */
                Index input  = INVALID_INDEX;
                Index output = INVALID_INDEX;
                Interpolation mode;

        } AnimationSampler;

        typedef struct
        {
                Index node = INVALID_INDEX;
                AnimationPath path;
                AnimationSampler sampler;
        } Channel;
};

struct Scene
{
        std::string name;
        std::vector<Index> rootNodes;
};

struct Model
{
        std::vector<Buffer> buffers;
        std::vector<BufferView> bufferViews;
        std::vector<Accessor> accessors;

        std::vector<ImageData> images;
        std::vector<Sampler> samplers;
        std::vector<Texture> textures;
        std::vector<Material> materials;

        std::vector<Mesh> meshes;
        std::vector<Node> nodes;
        std::vector<AssetCamera> cameras;
        std::vector<Skin> skins;
        std::vector<Scene> scenes;

        Index defaultScene = INVALID_INDEX;
        std::vector<std::string> extensions;
};

}  // namespace ic