#ifndef ENTITY_H
#define ENTITY_H

#include "defines.h"

#include "core/uuid.h"
#include "renderer/scene.h"

#include <entt/entt.hpp>

namespace ic
{
class RenderScene;

class Entity
{
private:
        string           name;
        ic::RenderScene *scene;
        entt::entity     handle;

public:
        Entity(entt::entity handle, ic::RenderScene *scene);
        Entity(const Entity &other) = default;

        const std::string &GetName() const { return name; }

        template <typename T, typename... Args>
        T &AddComponent(Args &&...args)
        {
                IC_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
                T &component = scene->m_Registry.emplace<T>(handle, std::forward<Args>(args)...);
                scene->OnComponentAdded<T>(*this, component);
                return component;
        }

        template <typename T, typename... Args>
        T &AddOrReplaceComponent(Args &&...args)
        {
                T &component = scene->m_Registry.emplace_or_replace<T>(handle, std::forward<Args>(args)...);
                scene->OnComponentAdded<T>(*this, component);
                return component;
        }

        template <typename T>
        T &GetComponent()
        {
                IC_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
                return scene->m_Registry.get<T>(handle);
        }

        template <typename T>
        bool HasComponent()
        {
                return scene->m_Registry.all_of<T>(handle);
        }

        template <typename T>
        void RemoveComponent()
        {
                IC_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
                scene->m_Registry.remove<T>(handle);
        }

        operator bool() const { return handle != entt::null; }
        operator entt::entity() const { return handle; }
        operator uint32_t() const { return (uint32_t)handle; }

        UUID               GetUUID() { return GetComponent<IDComponent>().ID; }
        const std::string &GetName() { return GetComponent<TagComponent>().Tag; }

        bool operator==(const Entity &other) const { return handle == other.handle && scene == other.scene; }

        bool operator!=(const Entity &other) const { return !(*this == other); }
};

}  // namespace ic

#endif  // ENTITY_H