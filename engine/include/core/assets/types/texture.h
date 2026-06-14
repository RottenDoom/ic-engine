#ifndef TEXTURE_H
#define TEXTURE_H

#include "asset_base.h"
#include "model.h"
#include "core/assets/asset_loaders/model_data.h"

namespace ic
{

struct GLTexture;

class Texture : public IAsset
{
public:
        ASSET_CLASS_TYPE(ASSET_TYPE_TEXTURE);

        Texture() = default;
        Texture(const TextureHandle id) : IAsset(id) {}
        ~Texture() override { Release(); }

        // Load texture from path into image struct for loading
        bool Load(const char *filepath) override;

        // Loads image data from Image data (used for loading textures through model pixels)
        void LoadFromImageData(ImageImportData &data);
        void SetSampler(SamplerImportData &sampler);

        // Set runtime image/sampler directly (used when reconstructing from a serialized model).
        void SetImage(Image image) { m_image = std::move(image); }
        void SetSampler(const Sampler &sampler) { m_sampler = sampler; }

        // Not implemented yet
        bool SerializedLoad(ic::Serializer *serializer) override;
        bool SerializedSave(ic::Serializer *Serializer) const override;

        // Release the resources after use.
        bool Release() override;

        Image   *GetImageTexture() { return &m_image; }
        Sampler &GetImageSampler() { return m_sampler; }

        // checks if the texture holds valid pixel data
        bool IsValid() const;

        // GL_SPECIFIC
        uint32_t GetGPUHandle() const;
        void     SetGPUHandle(GLTexture *gpuHandle);

private:
        Image   m_image;    // Texture pixels
        Sampler m_sampler;  // Sampler data this is not an asset for now

        /** NOTE: THIS IS GL_SPECIFIC ONLY FOR NOW */
        GLTexture *m_gpu = nullptr;
};

}  // namespace ic

#endif  // TEXTURE_H
