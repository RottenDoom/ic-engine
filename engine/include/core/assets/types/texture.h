#ifndef TEXTURE_H
#define TEXTURE_H

#include "asset_base.h"
#include "model.h"

namespace ic
{

class Texture : public IAsset
{
public:
        ASSET_CLASS_TYPE(ASSET_TYPE_TEXTURE);

        Texture() = default;
        Texture(const IC_GUID id) : IAsset(id) {}
        ~Texture() override { Release(); }

        // Load texture from path into image struct for loading
        bool Load(const char *filepath) override;

        // Note implemented yet
        bool SerializedLoad(ic::Serializer *serializer) override;
        bool SerializedSave(ic::Serializer *Serializer) const override;

        // Release the resources after use.
        bool Release() override;

        Image *GetImageTexture() { return m_image; }

private:
        Image *m_image = nullptr;  // Texture pixels
};

}  // namespace ic

#endif  // TEXTURE_H