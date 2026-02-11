#ifndef ASSET_MODEL_H
#define ASSET_MODEL_H

#include "defines.h"
#include "core/assets/types/asset_base.h"
#include "core/assets/asset_parser.h"

/** TODO: math libarry??? */
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

class IC_Model : public ic::IAsset
{
public:
        ASSET_CLASS_TYPE(ASSET_TYPE_MODEL)

        /** TODO: Implementation */
        bool Load(const GUID id) override { return false; }
        virtual bool CachedLoad(ic::ISerializer *serializer) override { return false; }
        virtual bool CachedSave(ic::ISerializer *serializer) const override { return false; }
        void Free() override {}
        void FreeCPU() {}
        void FreeGPU() {}

private:
        std::vector<Mesh> meshes;
        std::vector<AABB> meshAABBs;
};

/** TODO: implment this */
class AssetSerializer
{
};

class ModelSerializer : public AssetSerializer
{
public:
private:
        bool DeserializeBinary(IC_Model *model, const char *filepath);
        bool DeserializeGLTF(IC_Model *model, const char *filepath);
};

#endif