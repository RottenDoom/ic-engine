#include "core/assets/types/texture.h"
#include "core/filesystem.h"

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

        int      desiredChannels = 3;  // TODO: make this configurable
        int      width, height, nrChannels;
        uint8_t *data = stbi_load(fullpath, &width, &height, &nrChannels, desiredChannels);

        if (!data)
        {
                IC_CORE_ERROR("Could not load file: {}", fullpath);
                return false;
        }
        size_t size = static_cast<size_t>(width) * static_cast<size_t>(width) * 4;

        m_image           = (Image *)ic_malloc(sizeof(size));
        m_image->width    = static_cast<uint32_t>(width);
        m_image->height   = static_cast<uint32_t>(height);
        m_image->channels = static_cast<uint32_t>(nrChannels);
        m_image->srgb     = true;
        m_image->pixels.assign(data, data + size);
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
        m_image->pixels.clear();
        m_image->width    = 0;
        m_image->height   = 0;
        m_image->channels = 0;
        m_image->srgb     = false;
        ic_free(m_image);
        m_image = nullptr;
}

}  // namespace ic