#include "core/assets/asset_cache.h"
#include "core/assets/asset_manager.h"
#include "core/filesystem.h"

namespace ic
{
namespace AssetCache
{

void Init()
{
        // TODO
        // IMPLEMENT DEFAULT FOLDERS FOR PUTTING ALL THE FILES HONESTY THEY ARE WRITTEN IN THE Registry
}

uint64_t GetAssetTimeStamp(AssetType type, const GUID id)
{
        const char *filename  = AssetManager::Get()->getRegistry()->getCachePath(id);
        char       *full_path = fs_getfullpath(filename);
        return fs_getLastModificationTime(full_path);
}

bool CacheAsset(AssetType type, const GUID id, IAsset *asset)
{
        const char *filename  = AssetManager::Get()->getRegistry()->getCachePath(id);
        char       *full_path = fs_getfullpath(filename);
        try
        {
                ic::Serializer serializer;
                if (!serializer.openForWrite(full_path))
                {
                        return false;
                }
                if (!asset->serializedSave(&serializer))
                {
                        serializer.close();
                        fs_delete(full_path);
                        return false;
                }
                serializer.close();
        }
        catch (std::exception &e)
        {
                fs_delete(full_path);
                throw e;
        }
        return true;
}

uint8_t *GetCachedAssetRaw(AssetType type, const GUID id, size_t numBytes)
{
        numBytes                 = 0;
        const char    *filename  = AssetManager::Get()->getRegistry()->getCachePath(id);
        char          *full_path = fs_getfullpath(filename);
        ic::Serializer serializer;
        if (!serializer.openForRead(full_path))
        {
                return nullptr;
        }
        numBytes         = serializer.bytesLeft();
        uint8_t *ret_val = (uint8_t *)ic_malloc(sizeof(uint8_t) * numBytes);
        serializer.read(ret_val, numBytes);
        return ret_val;
}

}  // namespace AssetCache

}  // namespace ic
