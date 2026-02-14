#include "core/assets/types/model.h"
#include "core/gltf_loader.h"

bool Model::Load(const char *filepath)
{
        // TODO: USE OTHER LOADER BASED ON TYPE OF FILE
        ic::GLTFLoader loader;
        loader.loadModel(filepath, this);
}

bool Model::CachedLoad(ic::Serializer *serializer)
{
        return false;
}

bool Model::CachedSave(ic::Serializer *serializer) const
{
        return false;
}

void Model::Free() {}
