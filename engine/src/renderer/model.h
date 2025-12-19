#pragma once
#include "defines.h"

#include <string>  // [TODO] Make string class using std::vector or a custom dynamic array
#include <vector>

#include <glfw/glfw3.h>

#include <glm/glm.hpp>

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
                Mat4,
                UNKNOWN
        } type;

        /** OpenGL specific */
        GLenum componentType;

        /** TODO: min and max handling */

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
        int magFilter;
        int minFilter;
        int wrapS;
        int wrapT;
};

struct Texture
{
        Index image   = INVALID_INDEX;
        Index sampler = INVALID_INDEX;

        /** The images define the image
        data used for the texture.
        This data can be given via
        a URI that is the location of
        an image file, or by a
        reference to a bufferView
        and a MIME type that
        defines the type of the image
        data that is stored in the
        buffer view. */
};

struct Material
{
        /* for gltf defualt model is metallic roughness model */
        typedef struct
        {
                Index baseColorTextureIndex = INVALID_INDEX;
                Index baseColorTextureCoord = INVALID_INDEX;

                float baseColorFactor[4];  // RGBA

                Index metallicRoughnessTextureIndex = INVALID_INDEX;
                Index metallicRuughnessTextureCoord = INVALID_INDEX;

                float metallicFactor;
                float roughnessFactor;

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
        OcclusionTexture occlussion;
        EmissiveTexture emissive;
        float emissiveFactor[3];  // RGB
};

struct MeshPrimitive
{
        enum class Mode
        {
                POINTS,
                LINES,
                TRIANGLES
        } mode;

        Index positionAccessor = INVALID_INDEX;
        Index normalAccessor   = INVALID_INDEX;
        Index uvAccessor       = INVALID_INDEX;

        /** TODO: Skinning Matrix */
        Index jointAccessor  = INVALID_INDEX;
        Index weightAccessor = INVALID_INDEX;

        Index indexAccessor  = INVALID_INDEX;
        Index material       = INVALID_INDEX;

        // [TODO] targets
};

struct Mesh
{
        std::vector<MeshPrimitive> meshPrimitives;
        std::vector<float> weights; /* somehow related to morph targets */
};

struct Node
{
        std::string name;

        float translation[3];
        float rotation[4];
        float scale[3];

        float matrix[16];

        Index mesh   = INVALID_INDEX;
        Index skin   = INVALID_INDEX;

        Index camera = INVALID_INDEX;

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