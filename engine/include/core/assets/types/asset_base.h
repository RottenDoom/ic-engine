#ifndef ASSET_BASE_H
#define ASSET_BASE_H

#include "defines.h"
#include "core/assets/asset_serializer.h"

/** TODO:
 * 1. Asset Base UUID generator when writing a file to registry
 * 2. File hash functions and versioning when packing assets.
 */

/** TODO: Define this function */
#define HASH(x) 1997;

#define ASSET_CLASS_TYPE(type)                                                                                         \
        static AssetType getStaticType()                                                                               \
        {                                                                                                              \
                return AssetType::type;                                                                                \
        }                                                                                                              \
        virtual AssetType getAssetType() const override                                                                \
        {                                                                                                              \
                return getStaticType();                                                                                \
        }                                                                                                              \
        virtual const char *getName() const override                                                                   \
        {                                                                                                              \
                return #type;                                                                                          \
        }

using GUID = uint32_t;
extern const GUID INVALID_ID;

enum AssetType : uint8_t
{
        ASSET_TYPE_NONE = 0,
        //     ASSET_TYPE_GFX_IMAGE = 0,
        //     ASSET_TYPE_MATERIAL  = 1,
        //     ASSET_TYPE_SCRIPT    = 2,
        ASSET_TYPE_MODEL  = 1,  // 3,
        ASSET_TYPE_SHADER = 2,  // 4,
                                //     ASSET_TYPE_UI_LAYOUT = 5,
                                //     ASSET_TYPE_PIPELINE  = 6,
                                //     ASSET_TYPE_FONT      = 7,
                                //     ASSET_TYPE_NON_METADATA_COUNT = 8,
                                //     ASSET_TYPE_TEXTURESET = 8,

        ASSET_TYPE_COUNT
};

/**
 * IAsset Interface class. This internal class is a base class that gets inherited by most resources.
 * IAsset contains a asset type, a global unique id, and a reference count of it.
 */
class IAsset
{
public:
        IAsset() = default;
        IAsset(GUID id) : _id(id) {}
        IAsset(const char *name)
        {
                m_name = strdup(name);
                _id    = HASH(m_name);
        };
        virtual ~IAsset() { m_name = nullptr; }

        // Types
        virtual AssetType getAssetType() const = 0;
        virtual const char *getName() const    = 0;
        virtual string toString() const { return getName(); }

        virtual bool Load(const char *filepath) { return false; };
        virtual bool CachedLoad(ic::Serializer *serializer)       = 0;
        virtual bool CachedSave(ic::Serializer *serializer) const = 0;
        virtual void Free() {}

        void SetName(char *name) { m_name = name; };
        const char *GetName() { return m_name; }

        GUID GetID() { return _id; }

        void AddRef() { _ref_count++; }
        int32_t GetRefNum() { return _ref_count; }

private:
        char *m_name       = nullptr;
        GUID _id           = INVALID_ID;
        int32_t _ref_count = 0;
        void serializeName(ic::Serializer *serializer) const;
};

#endif