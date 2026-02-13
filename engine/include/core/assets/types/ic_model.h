#ifndef ASSET_MODEL_H
#define ASSET_MODEL_H

#include "defines.h"
#include "core/assets/types/asset_base.h"
#include "core/assets/asset_parser.h"

/** TODO:
 * 1.math libarry???
 * 2. Completely understand all the buffer types and load them into the scene with most optimized method.
 * 3. Complete the base asset as well as Mesh and AABB struct
 *  */

using vec3 = float[3];
using vec2 = float[2];
using vec4 = float[4];

struct AABB
{
};

struct Mesh
{
        // BUFFER TYPES COME HERE
#if defined(GPU_DATA)
        Buffer *buffers;
        u32 numVertices;
        u32 numMeshlets;
        bool hasTexCoords;
        bool unormTexCoords;
        bool hasTangents;
        u32 bindlessBuffersSlot;
#else

        std::vector<vec3> packedPositions;
        std::vector<vec3> packedNormals;
        std::vector<vec4> packedTangents;  // xyz is the tangent, w is the bitangent sign
        std::vector<vec2> packedTexCoords;

#endif
        // since the const char wouldn't be changeable I am going to use std::string for a while
        string name;
};

class IC_Model : public IAsset
{
public:
        ASSET_CLASS_TYPE(ASSET_TYPE_MODEL)

        /** TODO: Implementation */
        // MAYBE ADD FILENAME OR FILEPATH HERE
        bool Load(const GUID id) override;
        bool CachedLoad(Serializer *serializer) override { return false; }
        bool CachedSave(Serializer *serializer) const override { return false; }
        void Free() override {}
        void FreeCPU() {}
        void FreeGPU() {}

private:
        std::vector<Mesh> meshes;
        std::vector<AABB> meshAABBs;
};

#endif