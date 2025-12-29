#pragma once
#include "../model.h"
#include <glad/glad.h>

/** Gonna make a Material system here */

namespace ic
{

struct TextureInfo
{
        Index idx      = INVALID_INDEX;
        Index texCoord = INVALID_INDEX;
};

struct Texture
{
        TextureInfo textureInfo{};
        Index image   = INVALID_INDEX;
        Index sampler = INVALID_INDEX;
};

struct GLTexture
{
        GLuint textureHandle;
        void createTexture(Model& model, Texture& tex, ImageData& img);
        void applySampler(Sampler& sampler);
};

/** PBR Data */

struct PBRMaterial
{
        Texture baseColorTexture{};
        glm::vec4 baseColorFactor = glm::vec4(1.0f);

        Texture metallicRoughnessTexture{};
        float metallicFactor  = 1.0f;
        float roughnessFactor = 1.0f;
};

struct NormalTexture : public Texture
{
        float scale = 1.0f;
};

struct OcclusionTexture : public Texture
{
        float strength = 1.0f;
};

/** Other material types */
// Anisotropoc filtering
struct AnisotropicMaterial
{
        float strength = 0.0f;
        float rotation = 0.0f;
        Texture anisotropicTexture{};
};

struct DiffuseTransmissionMaterial
{
        float diffuseTransmissionFactor = 0.0f;
        Texture diffuseTransmissionTexture{};
        glm::vec3 diffuseTransmissionColorFactor = glm::vec3(1.0f);
        Texture diffuseTransmissionColorTexture{};
};

struct SpecularMaterial
{
        float specularFactor = 1.0f;
        Texture specularTexture{};
        glm::vec3 specularColorFactor = glm::vec3(1.0f);
        Texture specularColorTexture{};
};

struct IridescenceMaterial
{
        float iridescenceFactor = 0.0f;
        Texture iridescenceTexture{};
        float iridescenceIor              = 1.3f;
        float iridescenceThicknessMinimum = 100.0f;
        float iridescenceThicknessMaximum = 400.0f;
        Texture iridescenceThicknessTexture{};
};

/** TODO: Probably add more */

struct Material
{
        std::string name;

        PBRMaterial pbrMaterial{};
        NormalTexture normalTexture{};
        OcclusionTexture occlusionTexture{};
        Texture emissiveTexture{};

        glm::vec3 emissiveFactor = glm::vec3(0.0f);

        enum class AlphaMode : uint8_t
        {
                OPAQUE,
                MASK,
                BLEND
        };

        AlphaMode alphaMode    = AlphaMode::OPAQUE;
        bool doubleSided       = false;
        bool unlit             = false;

        float alphaCutoff      = 0.5f;
        float emissiveStrength = 1.0f;

        float ior              = 1.5f;
        float dispersion       = 0.0f;

        /** TODO: Add more material types here */

        Material* getDefaultMaterial();
};

}  // namespace ic