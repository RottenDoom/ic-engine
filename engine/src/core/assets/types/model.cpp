#include "core/assets/types/model.h"
#include "core/gltf_loader.h"

bool Model::load(const char *filepath)
{
        // TODO: USE OTHER LOADER BASED ON TYPE OF FILE
        char *fullpath = ic::fs_getfullpath(filepath);
        ic::GLTFLoader loader;
        loaded = true;
        return loader.loadModel(fullpath, this);
}

bool Model::cachedLoad(ic::Serializer *serializer)
{
        return false;
}

bool Model::cachedSave(ic::Serializer *serializer) const
{
        return false;
}

bool Model::release()
{
        return false;
}
