#include "core/assets/types/skybox.h"
#include "core/assets/asset_serializer.h"
#include "core/filesystem.h"

// Toggle: by default we use stb however sometimes ktx is used as I hate the dds format and works fine with vulkan
// formats.
#define IC_SKYBOX_USE_STB

#include <ktx.h>
#include <glad/glad.h>

#include <stb_image.h>

static void MapVkToGL(CubemapFormat &fmt, uint32_t vkFormat)
{
        switch (vkFormat)
        {
        case 145:                             // VK_FORMAT_BC7_UNORM_BLOCK
                fmt.internalFormat = 0x8E8C;  // GL_COMPRESSED_RGBA_BPTC_UNORM
                fmt.compressed     = true;
                break;
        case 146:                             // VK_FORMAT_BC7_SRGB_BLOCK
                fmt.internalFormat = 0x8E8D;  // GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
                fmt.compressed     = true;
                break;
        case 36:                              // VK_FORMAT_BC6H_SFLOAT_BLOCK
                fmt.internalFormat = 0x8E8E;  // GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
                fmt.compressed     = true;
                break;
        case 35:                              // VK_FORMAT_BC6H_UFLOAT_BLOCK
                fmt.internalFormat = 0x8E8F;  // GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
                fmt.compressed     = true;
                break;
        case 43:                              // VK_FORMAT_R8G8B8A8_UNORM
                fmt.internalFormat = 0x8058;  // GL_RGBA8
                fmt.externalFormat = 0x1908;  // GL_RGBA
                fmt.type           = 0x1401;  // GL_UNSIGNED_BYTE
                fmt.compressed     = false;
                break;
        case 50:                              // VK_FORMAT_R8G8B8A8_SRGB
                fmt.internalFormat = 0x8C43;  // GL_SRGB8_ALPHA8
                fmt.externalFormat = 0x1908;  // GL_RGBA
                fmt.type           = 0x1401;  // GL_UNSIGNED_BYTE
                fmt.compressed     = false;
                break;
        default:
                IC_CORE_ERROR("MapVkToGL -> unsupported vkFormat {}", vkFormat);
                break;
        }
}

bool Skybox::LoadFaceKTX(const std::string &path, CubemapFace *outFace)
{
        ktxTexture2   *texture  = nullptr;
        const char    *filepath = ic::fs_getfullpath(path.c_str());
        KTX_error_code result   = ktxTexture2_CreateFromNamedFile(filepath,
                                                                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                                                &texture);
        ic_free(filepath);

        if (result != KTX_SUCCESS || !texture)
        {
                IC_CORE_ERROR("Skybox::LoadFace -> failed to load '{}': {}", path, ktxErrorString(result));
                return false;
        }

        if (ktxTexture2_NeedsTranscoding(texture))
        {
                result = ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC7_RGBA, 0);
                if (result != KTX_SUCCESS)
                {
                        IC_CORE_ERROR("Skybox::LoadFace -> transcode failed '{}': {}", path, ktxErrorString(result));
                        ktxTexture_Destroy(ktxTexture(texture));
                        return false;
                }
        }

        uint8_t *data = ktxTexture_GetData(ktxTexture(texture));
        if (!data)
        {
                IC_CORE_ERROR("Skybox::LoadFace -> no image data in '{}'", path);
                ktxTexture_Destroy(ktxTexture(texture));
                return false;
        }

        uint32_t baseW = texture->baseWidth;
        uint32_t baseH = texture->baseHeight;
        uint32_t mips  = texture->numLevels;

        // Write shared state on first face only
        if (m_baseWidth == 0)
        {
                m_baseWidth  = baseW;
                m_baseHeight = baseH;
                m_mipLevels  = mips;
                MapVkToGL(m_format, texture->vkFormat);
        }

        outFace->mips.resize(mips);
        for (uint32_t level = 0; level < mips; level++)
        {
                ktx_size_t offset = 0;
                result            = ktxTexture_GetImageOffset(ktxTexture(texture), level, 0, 0, &offset);
                if (result != KTX_SUCCESS)
                {
                        IC_CORE_ERROR("Skybox::LoadFace -> offset failed level {} in '{}'", level, path);
                        ktxTexture_Destroy(ktxTexture(texture));
                        return false;
                }

                ktx_size_t imageSize = ktxTexture_GetImageSize(ktxTexture(texture), level);
                uint8_t   *src       = data + offset;

                outFace->mips[level].width  = std::max(1u, baseW >> level);
                outFace->mips[level].height = std::max(1u, baseH >> level);
                outFace->mips[level].data.resize(imageSize);
                std::memcpy(outFace->mips[level].data.data(), src, imageSize);
        }

        ktxTexture_Destroy(ktxTexture(texture));
        return true;
}

bool Skybox::LoadFaceSTB(const std::string &path, CubemapFace *outFace)
{
        const char *filepath = ic::fs_getfullpath(path.c_str());
        int         w, h, channels;
        uint8_t    *data = stbi_load(filepath, &w, &h, &channels, 4);
        ic_free(filepath);

        if (!data)
        {
                IC_CORE_ERROR("Skybox::LoadFaceSTB -> failed to load '{}': {}", path, stbi_failure_reason());
                return false;
        }

        if (m_baseWidth == 0)
        {
                m_baseWidth             = static_cast<uint32_t>(w);
                m_baseHeight            = static_cast<uint32_t>(h);
                m_mipLevels             = 1;
                m_format.internalFormat = 0x8058;  // GL_RGBA8
                m_format.externalFormat = 0x1908;  // GL_RGBA
                m_format.type           = 0x1401;  // GL_UNSIGNED_BYTE
                m_format.compressed     = false;
        }

        outFace->mips.resize(1);
        outFace->mips[0].width  = static_cast<uint32_t>(w);
        outFace->mips[0].height = static_cast<uint32_t>(h);
        size_t imageSize        = static_cast<size_t>(w) * h * 4;
        outFace->mips[0].data.resize(imageSize);
        std::memcpy(outFace->mips[0].data.data(), data, imageSize);

        stbi_image_free(data);
        return true;
}

bool Skybox::Release()
{
        for (auto &face : m_faces)
                face.mips.clear();
        m_format     = {};
        m_baseWidth  = 0;
        m_baseHeight = 0;
        m_mipLevels  = 0;
        m_state      = State::Unloaded;
        return true;
}

bool Skybox::Load(const char *filepath)
{
        IC_CORE_ASSERT(filepath, "Skybox::Load -> null filepath");

        if (m_state == State::Ready)
        {
                IC_CORE_WARN("Skybox::Load -> already loaded, call release() first");
                return false;
        }

        std::string dir = filepath;
        if (!dir.empty() && dir.back() != '/')
                dir += '/';

#ifdef IC_SKYBOX_USE_STB
        const auto &faceNames = k_faceNamesPNG;
#else
        const auto &faceNames = k_faceNamesKTX;
#endif

        for (int i = 0; i < 6; i++)
        {
                std::string path = dir + faceNames[i];
#ifdef IC_SKYBOX_USE_STB
                if (!LoadFaceSTB(path, &m_faces[i]))
#else
                if (!LoadFaceKTX(path, &m_faces[i]))
#endif
                {
                        m_state = State::Failed;
                        return false;
                }

                if (i > 0)
                {
                        if (m_faces[i].mips[0].width != m_baseWidth || m_faces[i].mips[0].height != m_baseHeight)
                        {
                                IC_CORE_ERROR("Skybox::Load -> face {} dimension mismatch "
                                              "(got {}x{}, expected {}x{})",
                                              i,
                                              m_faces[i].mips[0].width,
                                              m_faces[i].mips[0].height,
                                              m_baseWidth,
                                              m_baseHeight);
                                m_state = State::Failed;
                                return false;
                        }
                }
        }

        m_state = State::Ready;
        IC_CORE_INFO("Skybox::Load -> '{}' ready ({}x{}, {} mips)", filepath, m_baseWidth, m_baseHeight, m_mipLevels);
        return true;
}

bool Skybox::SerializedLoad(ic::Serializer *serializer)
{
        IC_CORE_ASSERT(serializer && serializer->isReading(), "SerializedLoad: serializer not open for read");

        IC_CORE_WARN("Skybox serialization not implemented yet");
        return false;
}

bool Skybox::SerializedSave(ic::Serializer *serializer) const
{
        IC_CORE_ASSERT(serializer && serializer->isReading(), "SerializedLoad: serializer not open for read");

        IC_CORE_WARN("Skybox serialization not implemented yet");
        // ISSUE: we do not have a compression library to fit the whole scene like this so cant really use this
        serializer->write("baseWidth", m_baseWidth);
        serializer->write("baseHeight", m_baseHeight);
        serializer->write("mipLevels", m_mipLevels);
        return false;
}
