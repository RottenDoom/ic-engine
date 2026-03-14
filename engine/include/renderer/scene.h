#ifndef SCENE_H
#define SCENE_H

#include "defines.h"
#include "camera.h"
#include "core/assets/types/asset_base.h"
#include "core/ecs/components.h"
#include "core/application.h"

#include <entt/entt.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// makes life easier.
#define NULL_ENTITY { entt::null, nullptr }

/** Scene class that can be setup by user or anyone. */
namespace ic
{

class Entity;

class IC_API RenderScene
{
public:
        // Default camera see if this can be improved with a better entity class
        Camera defaultCamera;

        friend class Entity;

public:
        RenderScene();
        ~RenderScene();

        Entity CreateEntityWithName(const string &name = string());
        Entity CreateEntity(UUID id, const string &name = string());
        void   DestroyEntity(Entity entity);

        template <typename T>
        void OnComponentAdded(Entity entity, T &component);
        void DrawScene(Camera &editorCamera);

        // void OnUpdateRuntime(Timestep ts);
        // void OnUpdateSimulation(Timestep ts, EditorCamera &camera);
        // void OnUpdateEditor(Timestep ts, EditorCamera &camera);
        // void OnViewportResize(uint32_t width, uint32_t height);

        Entity FindEntityByName(std::string_view name);  // we use string view when we dont wanna story the memory so
                                                         // use them as params

        Entity GetEntityByUUID(UUID uuid);

        bool IsRunning() const { return m_IsRunning; }
        bool IsPaused() const { return m_IsPaused; }

        void SetPaused(bool paused) { m_IsPaused = paused; }

private:
        entt::registry m_Registry;
        uint32_t       m_ViewportWidth = 0, m_ViewportHeight = 0;
        bool           m_IsRunning = false;
        bool           m_IsPaused  = false;

        std::unordered_map<UUID, entt::entity> m_EntityMap;
};

}  // namespace ic

#endif