#pragma once
#include "renderer/light_types.h"
#include "renderer/opengl/light_ubo.h"
#include "core/ecs/components.h"

class Camera;

namespace ic
{

class RenderScene;

class LightSystem
{
public:
        // Call once.
        void Init(LightUBO *ubo);

        // Call every frame before any draw call.
        // Reads all LightComponent + TransformComponent pairs from the scene,
        // packs them into GPULight, and uploads the UBO.
        void Update(RenderScene *scene, const Camera &camera, float time);

private:
        LightUBO *m_ubo = nullptr;

        GPULight PackLight(const LightComponent &lc, const TransformComponent &tc) const;
};

}  // namespace ic