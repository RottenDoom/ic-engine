#ifndef SCENE_H
#define SCENE_H

#include "defines.h"
#include "camera.h"
#include "core/assets/types/asset_base.h"
#include "core/ecs/entity.h"
#include "core/ecs/components.h"
#include "core/application.h"

#include <entt/entt.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/** Scene class that can be setup by user or anyone. */
namespace ic
{
class IC_API RenderScene
{
public:
        // Default camera see if this can be improved with a better entity class
        Camera defaultCamera;

public:
        RenderScene();
        ~RenderScene();

        Entity createEntity(const string &name = string());
        Entity createEntity(UUID id, const string &name = string());
        void   destroyEntity(Entity entity);

        MeshComponent      &addMesh(Entity entity) { return m_Registry.emplace<MeshComponent>(entity); }
        TransformComponent &addTransform(Entity entity) { return m_Registry.emplace<TransformComponent>(entity); }
        CameraComponent    &addCamera(Entity entity) { return m_Registry.emplace<CameraComponent>(entity); }
        LightComponent     &addLight(Entity entity) { return m_Registry.emplace<LightComponent>(entity); }

        template <typename T>
        void onComponentAdded(Entity entity, T &component);
        void renderScene(Camera &editorCamera);

        Entity findEntityByName(string &name);
        Entity getEntityByUUID(UUID uuid);

        bool isRunning() const { return m_IsRunning; }
        bool isPaused() const { return m_IsPaused; }

        void setPaused(bool paused) { m_IsPaused = paused; }

private:
        entt::registry m_Registry;
        uint32_t       m_ViewportWidth = 0, m_ViewportHeight = 0;
        bool           m_IsRunning = false;
        bool           m_IsPaused  = false;

        std::unordered_map<UUID, entt::entity> m_EntityMap;
};

}  // namespace ic

#endif