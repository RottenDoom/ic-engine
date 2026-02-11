#ifndef ASSET_BASE_H
#define ASSET_BASE_H
#include "defines.h"

/** TODO: Define this function */
#define HASH(x) 1997;

#define ASSET_CLASS_TYPE(type)                                                                                         \
        static ic::AssetType getStaticType()                                                                           \
        {                                                                                                              \
                return ic::AssetType::type;                                                                            \
        }                                                                                                              \
        virtual ic::AssetType getAssetType() const override                                                            \
        {                                                                                                              \
                return getStaticType();                                                                                \
        }                                                                                                              \
        virtual const char *getName() const override                                                                   \
        {                                                                                                              \
                return #type;                                                                                          \
        }

using GUID = uint32_t;
extern const GUID INVALID_ID;

namespace ic
{

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

class ISerializer
{
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

        virtual bool Load(const GUID id) { return false; };
        virtual bool CachedLoad(ISerializer *serializer)       = 0;
        virtual bool CachedSave(ISerializer *serializer) const = 0;
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
        // void SerializeName(ISerializer *serializer) const;
};

}  // namespace ic

#endif