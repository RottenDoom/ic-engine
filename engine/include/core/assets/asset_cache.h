#ifndef ASSET_CACHE_H
#define ASSET_CACHE_H

#include "defines.h"
#include "asset_serializer.h"
#include "types/asset_base.h"

// TODO:
/**
 * 1. Complete the implementation of cached loads and cached assets and test it
 * 2. Timestamp and fix the filesystem mounting logic better instead of using the full path function everytime. Its time
 * consuming and not cool
 * 3. Correctly implement cacheasset and asset raw here.
 */

namespace ic
{

namespace AssetCache
{

void Init();
uint64_t GetAssetTimeStamp(AssetType type, const GUID id);
bool CacheAsset(AssetType type, const GUID id, IAsset *asset);
uint8_t *GetCachedAssetRaw(AssetType type, const GUID id, size_t numBytes);

}  // namespace AssetCache

}  // namespace ic

#endif