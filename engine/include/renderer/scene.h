#ifndef SCENE_H
#define SCENE_H

#include "defines.h"
#include "camera.h"
#include "core/uuid.h"
#include "core/ecs/entity.h"
#include "core/ecs/components.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#define NULL_ENTITY ic::Entity{}

namespace ic
{

class IC_API RenderScene
{
public:
        friend class Entity;

        RenderScene();
        ~RenderScene();

        Entity CreateEntityWithName(const string &name = string());
        Entity CreateEntity(UUID id, const string &name = string());
        void   DestroyEntity(Entity entity);
        bool   SetParent(Entity child, Entity parent);
        bool   ClearParent(Entity child);

        Entity              GetParent(Entity e);
        std::vector<Entity> GetChildren(Entity e);
        std::vector<Entity> GetRoots();
        std::vector<Entity> GetAllEntities();

        // walks the parent chain multiplying transforms
        glm::mat4 GetWorldTransform(Entity e);

        bool   HasEntity(UUID uuid) const;
        Entity FindEntityByName(std::string_view name);
        Entity GetEntityByUUID(UUID uuid);

        bool IsRunning() const { return m_IsRunning; }
        bool IsPaused() const { return m_IsPaused; }
        void SetPaused(bool p) { m_IsPaused = p; }

        // --------------------------------------------------
        // Template queries — defined inline here so the
        // compiler can instantiate them in any translation unit
        // --------------------------------------------------
        template <typename... T, typename Func>
        void Each(Func &&fn)
        {
                m_Registry.view<T...>().each(std::forward<Func>(fn));
        }

        // walks the whole subtree rooted at e, calls fn on each
        template <typename Func>
        void EachInSubtree(Entity root, Func &&fn)
        {
                fn(root);
                if (!root.HasComponent<HierarchyComponent>())
                        return;
                for (auto childHandle : root.GetComponent<HierarchyComponent>().children)
                {
                        EachInSubtree({childHandle, this}, fn);
                }
        }

        template <typename... T>
        std::vector<Entity> GetEntitiesWith()
        {
                std::vector<Entity> result;
                for (auto handle : m_Registry.view<T...>())
                        result.emplace_back(handle, this);
                return result;
        }

        template <typename T>
        Entity GetFirstWith()
        {
                auto view = m_Registry.view<T>();
                if (view.begin() == view.end())
                        return Entity{};
                return Entity(*view.begin(), this);
        }

        // Base template function can be defined for each class internal struct.
        template <typename T>
        void OnComponentAdded(Entity entity, T &component)
        {
        }

private:
        entt::registry                         m_Registry;
        uint32_t                               m_ViewportWidth  = 0;
        uint32_t                               m_ViewportHeight = 0;
        bool                                   m_IsRunning      = false;
        bool                                   m_IsPaused       = false;
        std::unordered_map<UUID, entt::entity> m_EntityMap;
};

template <>
inline void RenderScene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent &component)
{
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
                component.camera.SetViewPortSize(m_ViewportWidth, m_ViewportHeight);
}

}  // namespace ic
#endif