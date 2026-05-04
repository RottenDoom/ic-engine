#include "renderer/lighting_system.h"
#include "core/ecs/entity.h"
#include "core/ecs/entity_impl.h"
#include "renderer/camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace ic
{

void LightSystem::Init(LightUBO *ubo)
{
        m_ubo = ubo;
}

void LightSystem::Update(RenderScene *scene, const Camera &camera, float time)
{
        if (!m_ubo || !scene)
                return;

        // --- Per-frame data (camera) ---
        PerFrameData frame{};
        frame.view       = camera.matrices.view;
        frame.projection = camera.projection;
        frame.cameraPos  = glm::vec4(camera.position, 1.0f);
        frame.time       = time;
        m_ubo->UploadPerFrame(frame);

        // --- Light data ---
        LightBlockData block{};
        block.lightCount = 0;

        auto lightEntities = scene->GetEntitiesWith<LightComponent, TransformComponent>();
        for (Entity &e : lightEntities)
        {
                if (block.lightCount >= MAX_LIGHTS)
                        break;

                auto &lc = e.GetComponent<LightComponent>();
                auto &tc = e.GetComponent<TransformComponent>();

                if (!lc.enabled)
                        continue;

                block.lights[block.lightCount++] = PackLight(lc, tc);
        }

        m_ubo->UploadLights(block);
}

GPULight LightSystem::PackLight(const LightComponent &lc, const TransformComponent &tc) const
{
        GPULight g{};

        glm::vec3 pos = tc.position;

        // For directional lights, position direction calc on GPU.
        glm::vec3 forward = glm::normalize(tc.rotation * glm::vec3(0.0f, 0.0f, -1.0f));

        g.position  = glm::vec4(pos, static_cast<float>(lc.type));
        g.direction = glm::vec4(forward, lc.range);
        g.color     = glm::vec4(lc.color, lc.intensity);

        // Spot angles
        float cosInner = std::cos(glm::radians(lc.innerAngle));
        float cosOuter = std::cos(glm::radians(lc.outerAngle));
        g.spotAngles   = glm::vec4(cosInner, cosOuter, 0.0f, 0.0f);

        return g;
}

}  // namespace ic