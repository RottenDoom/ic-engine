#include "renderer/opengl/gpu_resource_cache.h"
#include "core/assets/asset_manager.h"
#include "core/assets/types/model.h"

namespace ic
{

GPUResourceCache::~GPUResourceCache()
{
        // Note: clear() must be called by the renderer before this destructor runs,
        // while an OpenGL context is still active.
        // If it wasn't, GLModel destructors will call clearGPUMemory() here,
        // which is only safe if the context is still alive.
        m_models.clear();
        m_materials.clear();
}

GLModel *GPUResourceCache::GetOrUpload(const IC_GUID id, AssetManager &mgr)
{
        auto it = m_models.find(id);
        if (it != m_models.end())
                return it->second.get();

        // ISSUE: Multiple loads increasing the refcounts
        Model *model = mgr.LoadAs<Model>(id);
        if (!model)
        {
                IC_CORE_WARN("GPUResourceCache::GetOrUpload -> model {} not found in AssetManager", id);
                return nullptr;
        }

        if (!model->isCPUReady())
        {
                IC_CORE_WARN("GPUResourceCache::GetOrUpload -> model {} not CPU-ready (state={})",
                             id,
                             static_cast<int>(model->getState()));
                return nullptr;
        }

        auto glModel = std::make_unique<GLModel>();
        glModel->Upload(*model);

        GLModel *ptr = glModel.get();
        m_models.emplace(id, std::move(glModel));

        IC_CORE_INFO("GPUResourceCache: uploaded model {} to GPU", id);
        return ptr;
}

// If GLMaterial is not initialized it initializes it
GLMaterial *GPUResourceCache::GetCachedMaterial(const IC_GUID modelId, const MaterialHandle handle, AssetManager &mgr)
{
        // MaterialKey key{modelId, handle};

        // auto it = m_materials.find(key);
        // if (it != m_materials.end())
        //         return &it->second;

        // MaterialAsset *asset = mgr.GetAsset<MaterialAsset>(handle);
        // if (!asset)  // TODO: validation on material completeness
        // {
        //         IC_CORE_WARN("GPUResourceCache::GetMaterial -> material {} not found in model {}", handle, modelId);
        //         return nullptr;
        // }

        // GLMaterial &glMat = m_materials[key];
        // glMat.Build(asset->GetMaterial());
        // return &glMat;

        MaterialAsset *asset = mgr.GetAsset<MaterialAsset>(handle);  // contains GLmaterial* and Material

        if (!asset)
        {
                IC_CORE_WARN("GPUResourceCache::GetMaterial -> material {} not found in model {}", handle, modelId);
                return nullptr;
        }

        if (!asset->GetGPUHandle() || asset->IsDirty())
                asset->Rebake();

        return asset->GetGPUHandle();
}

void GPUResourceCache::Clear()
{
        // GLModel destructors call clearGPUMemory() — requires active GL context.
        m_models.clear();
        m_materials.clear();
}

}  // namespace ic
