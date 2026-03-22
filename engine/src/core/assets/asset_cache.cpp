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

uint64_t GetAssetTimeStamp(AssetType type, const IC_GUID id)
{
        const char *filename = AssetManager::Get().GetRegistry()->GetCachePath(id);

        if (!fs_exists(filename))
        {
                IC_CORE_ERROR("File path does not exist");
                ic_free(filename);
                return 0;
        }

        uint64_t timestamp = fs_getLastModificationTime(filename);
        ic_free(filename);
        return timestamp;
}

bool CacheAsset(AssetType type, const IC_GUID id, IAsset *asset)
{
        const char *filename  = AssetManager::Get().GetRegistry()->GetCachePath(id);
        const char *full_path = fs_getfullpath(filename);  /// fix this buillshit

        ic_free(filename);
        ic::Serializer serializer;
        if (!serializer.openForWrite(full_path))
        {
                IC_CORE_ERROR("Could not open file path {}.", full_path);
                ic_free(full_path);
                return false;
        }
        if (!asset->serializedSave(&serializer))
        {
                IC_CORE_ERROR("Could not save file {}", full_path);
                serializer.close();
                ic_free(full_path);
                return false;
        }
        ic_free(full_path);
        serializer.close();
        return true;
}

uint8_t *GetCachedAssetRaw(AssetType type, const IC_GUID id, size_t numBytes)
{
        numBytes              = 0;
        const char *filename  = AssetManager::Get().GetRegistry()->GetCachePath(id);
        char       *full_path = fs_getfullpath(filename);
        ic_free(filename);

        ic::Serializer serializer;
        if (!serializer.openForRead(full_path))
        {
                ic_free(full_path);
                return nullptr;
        }

        numBytes         = serializer.bytesLeft();
        uint8_t *ret_val = (uint8_t *)ic_malloc(sizeof(uint8_t) * numBytes);
        serializer.read(ret_val, numBytes);
        ic_free(full_path);
        return ret_val;
}

}  // namespace AssetCache

}  // namespace ic
