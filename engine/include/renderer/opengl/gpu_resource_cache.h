#ifndef GPU_RESOURCE_CACHE_H
#define GPU_RESOURCE_CACHE_H

#include "defines.h"
#include "renderer/opengl/gl_model.h"
#include "renderer/opengl/gl_material.h"

#include <unordered_map>
#include <memory>

namespace ic
{

class AssetManager;
class Model;

// ---------------------------------------------------------------------------
// GPUResourceCache -> owns all GPU-side model and material objects
//
// Replaces the raw m_gpuCache in OpenGLRenderer.
//
// GLModel* lifetime: owned via unique_ptr; destructor calls clearGPUMemory().
//   clear() must be called while an OpenGL context is active.
//
// GLMaterial lifetime: stored by value; contains no GL handles (pure uniforms).
//   Safe to destroy without an active context.
// ---------------------------------------------------------------------------

// TODO: this needs to change
struct MaterialKey
{
        IC_GUID        modelId;
        MaterialHandle handle;
        bool           operator==(const MaterialKey &o) const = default;
};

struct MaterialKeyHash
{
        size_t operator()(const MaterialKey &k) const noexcept
        {
                size_t h  = std::hash<IC_GUID>{}(k.modelId);
                h        ^= std::hash<uint32_t>{}(static_cast<uint32_t>(k.handle)) + 0x9e3779b9u + (h << 6) + (h >> 2);
                return h;
        }
};

class GPUResourceCache
{
public:
        ~GPUResourceCache();

        /**
         * Return the cached GLModel for id, or upload it from AssetManager if missing.
         * Returns nullptr if the asset is not found or not CPU-ready.
         */
        GLModel *GetOrUpload(const IC_GUID id, AssetManager &mgr);

        /**
         * Return the cached GLMaterial for (modelId, handle).
         * Builds and caches on first access.
         * Returns nullptr if materialHandle does not exist in assetmanager.
         */
        GLMaterial *GetCachedMaterial(const IC_GUID modelId, const MaterialHandle materialHandle, AssetManager &mgr);

        /** Delete all GPU resources. Must be called with an active OpenGL context. */
        void Clear();

private:
        std::unordered_map<IC_GUID, std::unique_ptr<GLModel>>        m_models;
        std::unordered_map<MaterialKey, GLMaterial, MaterialKeyHash> m_materials;
};

}  // namespace ic

#endif  // GPU_RESOURCE_CACHE_H
