#ifndef ASSET_SERIALIZED_H
#define ASSET_SERIALIZED_H

#include "defines.h"

// Take versioning timestamping and other cases and implement here along with the structure taken from the gltf files.

struct SerializedMesh
{
        // versions
        // magic no.
        // other things
        // offsets

        uint32_t vertex_count;
        uint32_t index_count;
        float *positions;
        float *normals;
        float *uvs;
        uint32_t *indices;
};

struct SerializedModel
{
        uint32_t mesh_count;
        SerializedMesh *meshes;
};

#endif