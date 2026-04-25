#ifndef SKYBOX_H
#define SKYBOX_H

#include "defines.h"
#include "core/assets/types/asset_base.h"

typedef uint32_t GLenum;
typedef uint32_t GLuint;

struct CubemapFaceLevel
{
        uint32_t             width;
        uint32_t             height;
        std::vector<uint8_t> data;
};

struct CubemapFace
{
        std::vector<CubemapFaceLevel> mips;
};

struct CubemapFormat
{
        uint32_t internalFormat;
        uint32_t externalFormat;
        uint32_t type;
        bool     compressed;
};

/** @brief Skybox is a loader as well as asset handling class for Skyboxes.
 * @bug Only works for dds files with specific names for now. Will be making more things but for now a fast working
 * concept has been made.
 */
class Skybox : public IAsset
{
public:
        ASSET_CLASS_TYPE(ASSET_TYPE_SKYBOX);

        enum class State : uint8_t
        {
                Unloaded,
                Ready,
                Failed
        };

        Skybox() = default;
        Skybox(IC_GUID id) : IAsset(id) {}
        ~Skybox() override { Release(); }

        bool Load(const char *filepath) override;
        bool SerializedLoad(ic::Serializer *serializer) override;
        bool SerializedSave(ic::Serializer *serializer) const override;

        bool Release() override;

        const std::array<CubemapFace, 6> &GetFaces() const { return m_faces; }
        const CubemapFormat              &GetFormat() const { return m_format; }
        uint32_t                          GetMipLevels() const { return m_mipLevels; }
        uint32_t                          GetBaseWidth() const { return m_baseWidth; }
        uint32_t                          GetBaseHeight() const { return m_baseHeight; }
        bool                              IsLoaded() const override { return m_state == State::Ready; }

private:
        bool LoadFaceKTX(const string &path, CubemapFace *outFace);
        bool LoadFaceSTB(const string &path, CubemapFace *outFace);

        static constexpr std::array<const char *, 6> k_faceNamesKTX = {
            "right.ktx2", "left.ktx2", "top.ktx2", "bottom.ktx2", "front.ktx2", "back.ktx2"};
        static constexpr std::array<const char *, 6> k_faceNamesPNG = {
            "right.png", "left.png", "top.png", "bottom.png", "front.png", "back.png"};

        std::array<CubemapFace, 6> m_faces;
        CubemapFormat              m_format     = {};
        uint32_t                   m_baseWidth  = 0;
        uint32_t                   m_baseHeight = 0;
        uint32_t                   m_mipLevels  = 0;
        State                      m_state      = State::Unloaded;
};

#endif