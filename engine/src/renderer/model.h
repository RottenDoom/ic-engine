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

#include <glad/glad.h>

namespace ic
{

using Index                   = uint32_t;
constexpr Index INVALID_INDEX = ~0u;

struct TextureInfo;
struct Texture;
struct Material;

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

        /** OpenGL specific */
        GLenum componentType = GL_BYTE;

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
        glm::vec2 uv2;
        glm::vec4 color;
        glm::vec4 tangent;
        glm::uvec4 joint0;
        glm::vec4 weight0;
        /* any other things*/
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

        // Raw decoded pixels (RGBA8, etc.)
        std::vector<uint8_t> pixels;

        // Optional metadata
        bool srgb = false;
};

struct Scene
{
        std::string name;
        std::vector<Index> rootNodes;
};

IC_API struct Model
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