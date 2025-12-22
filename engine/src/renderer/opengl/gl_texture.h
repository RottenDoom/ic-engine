#pragma once
#include "renderer/model.h"
#include <glad/glad.h>

namespace ic
{

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
        Index idx      = INVALID_INDEX;
        Index image    = INVALID_INDEX;
        Index sampler  = INVALID_INDEX;
        Index texCoord = INVALID_INDEX;
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
                Texture baseColorTexture{};

                glm::vec4 baseColorFactor;  // RGBA

                Texture metallicRoughnessTexture;

                float metallicFactor  = 1.0f;
                float roughnessFactor = 1.0f;

        } PbrMetallicRoughness;

        typedef struct
        {
                float scale = 1.0f;
                Texture normalTexture;
        } NormalTexture;

        typedef struct
        {
                float strength = 1.0f;
                Texture occlusionTexture;
        } OcclusionTexture;

        PbrMetallicRoughness pbr;
        NormalTexture normal;
        OcclusionTexture occlusion;
        Texture emissive;

        glm::vec3 emissiveFactor;  // RGB

        bool doubleSided = false;

        std::string name;
};

struct GLTexture
{
        GLuint textureHandle;
        void createTexture(Model& model, Texture& tex, ImageData& img);
        void applySampler(Sampler& sampler);
};

struct GLMaterial
{
};

}  // namespace ic