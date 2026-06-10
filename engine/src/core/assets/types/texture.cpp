#include "core/assets/types/texture.h"
#include "core/filesystem.h"
#include "core/assets/asset_loaders/model_data.h"

#include <stb_image.h>

namespace ic
{
bool Texture::Load(const char *filepath)
{
        IC_CORE_ASSERT(filepath, "Filepath is a nullptr");
        const char *fullpath = fs_getfullpath(filepath);
        if (!fullpath)
        {
                IC_CORE_ERROR("Path: {} does not exist!", filepath);
                return false;
        }

        const int desiredChannels = 4;  // engine convention: always decode to RGBA8
        int       width, height, nrChannels;
        uint8_t  *data = stbi_load(fullpath, &width, &height, &nrChannels, desiredChannels);

        if (!data)
        {
                IC_CORE_ERROR("Could not load file: {}", fullpath);
                ic_free(fullpath);
                return false;
        }
        const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * desiredChannels;

        m_image.width    = static_cast<uint32_t>(width);
        m_image.height   = static_cast<uint32_t>(height);
        m_image.channels = static_cast<uint32_t>(nrChannels);
        m_image.srgb     = true;
        m_image.pixels.assign(data, data + size);
        stbi_image_free(data);

        ic_free(fullpath);
        return true;
}

bool Texture::SerializedLoad(ic::Serializer *serializer)
{
        IC_CORE_INFO("Not implemented yet");
        return false;
}

bool Texture::SerializedSave(ic::Serializer *serializer) const
{
        IC_CORE_INFO("Not implemented yet");
        return false;
}

bool Texture::Release()
{
        m_image.pixels.clear();
        m_image.pixels.shrink_to_fit();
        m_image.width    = 0;
        m_image.height   = 0;
        m_image.channels = 0;
        m_image.srgb     = false;
        return true;
}

void Texture::LoadFromImageData(ImageImportData &data)
{
        m_image.name     = data.name;
        m_image.fromGLTF = true;
        m_image.width    = data.width;
        m_image.height   = data.height;
        m_image.channels = data.channels;
        m_image.srgb     = data.srgb;
        m_image.pixels   = std::move(data.pixels);  // moving the pixels instead of creating a copy
        m_image.uri      = data.uri;
}

void Texture::SetSampler(SamplerImportData &sampler)
{
        m_sampler.magFilter = static_cast<Sampler::Filter>(sampler.magFilter);
        m_sampler.minFilter = static_cast<Sampler::Filter>(sampler.minFilter);
        m_sampler.wrapS     = static_cast<Sampler::Wrap>(sampler.wrapS);
        m_sampler.wrapT     = static_cast<Sampler::Wrap>(sampler.wrapT);
}

bool Texture::IsValid() const
{
        // TODO: checkks for sampler as well.
        return !m_image.pixels.empty();
}

}  // namespace ic