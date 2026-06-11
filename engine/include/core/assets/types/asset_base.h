#ifndef ASSET_BASE_H
#define ASSET_BASE_H

#include "defines.h"
#include "core/uuid.h"

namespace ic
{
class Serializer;

}  // namespace ic

// TODO: use this in only modelimportdata eventually
using Index                   = uint32_t;
constexpr Index INVALID_INDEX = ~0u;

#define ASSET_CLASS_TYPE(type)                                                                                         \
        static AssetType GetStaticType()                                                                               \
        {                                                                                                              \
                return AssetType::type;                                                                                \
        }                                                                                                              \
        virtual AssetType GetAssetType() const override                                                                \
        {                                                                                                              \
                return GetStaticType();                                                                                \
        }                                                                                                              \
        virtual const char *GetAssetName() const override                                                                   \
        {                                                                                                              \
                return #type;                                                                                          \
        }

using IC_GUID                 = uint64_t;
constexpr uint64_t INVALID_ID = 0;

enum AssetType : uint8_t
{
        ASSET_TYPE_NONE = 0,
        ASSET_TYPE_MODEL,
        ASSET_TYPE_SHADER,
        ASSET_TYPE_TEXTURE,
        ASSET_TYPE_MATERIAL,
        ASSET_TYPE_SKYBOX,
        // ASSET_TYPE_GFX_IMAGE,
        // ASSET_TYPE_SCRIPT,
        // ASSET_TYPE_UI_LAYOUT,
        // ASSET_TYPE_PIPELINE,
        // ASSET_TYPE_FONT,
        // ASSET_TYPE_NON_METADATA_COUNT,

        ASSET_TYPE_COUNT
};

struct AssetMeta
{
        IC_GUID   id;
        string    name;
        string    filepath;
        string    cachePath;
        AssetType type;
};

/**
 * IAsset Interface class. This internal class is a base class that gets inherited by most resources.
 * IAsset contains a asset type, a global unique id, and a reference count of it.
 */
class IAsset
{
public:
        IAsset() = default;
        IAsset(IC_GUID id) : _id(id) {}

        IAsset(const char *name)
        {
                m_name = name;
                _id    = ic::UUIDGenerator::Generate();
        };

        virtual ~IAsset() {}

        // Type Signatures Load this using ASSET_CLASS_TYPE(type)
        virtual AssetType   GetAssetType() const = 0;
        virtual const char *GetAssetName() const = 0;
        virtual string      toString() const { return GetAssetName(); }

        // Implementation details to be implemented
        virtual bool Load(const char *filepath) { return false; };
        virtual bool SerializedLoad(ic::Serializer *serializer)       = 0;
        virtual bool SerializedSave(ic::Serializer *serializer) const = 0;
        virtual bool Release() { return false; }

        virtual bool IsLoaded() const { return loaded; }

        // Identifiable
        IC_GUID     GetID() const { return _id; }
        void        SetName(const char *name) { m_name = name; };
        const char *GetName() const { return m_name.c_str(); }

        // RefCountable
        void    AddRef() { _ref_count++; }
        void    RemoveRef() { _ref_count--; }
        int32_t GetRefNum() const { return _ref_count; }

private:
        bool loaded = false;

        // Identifiable
        IC_GUID _id = INVALID_ID;
        string  m_name;

        // Refcountable - Not every object is refcountable so maybe add refcountable object but we are not worried right
        // now until we get an asset like that. Probably Shader would be like that.
        int32_t _ref_count = 0;
        // void    serializeName(ic::Serializer *serializer) const;
};

#endif