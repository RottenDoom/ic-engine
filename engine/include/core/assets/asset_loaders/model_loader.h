#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "defines.h"
#include "model_data.h"

namespace ic
{
// Model Import Data

class IModelLoader
{
public:
        virtual ~IModelLoader()                                       = default;
        virtual bool canLoad(const char *ext) const                   = 0;
        virtual bool load(const char *path, ModelImportData *outData) = 0;
};
}  // namespace ic

#endif