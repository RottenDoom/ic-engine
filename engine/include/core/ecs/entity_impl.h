#ifndef ENTITY_IMPL_H
#define ENTITY_IMPL_H

#include "core/ecs/entity.h"
#include "renderer/scene.h"

namespace ic
{

template <typename T, typename... Args>
T &Entity::AddComponent(Args &&...args)
{
        IC_CORE_ASSERT(IsValid(), "Invalid entity!");
        IC_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
        T &component = scene->m_Registry.emplace<T>(handle, std::forward<Args>(args)...);
        scene->OnComponentAdded<T>(*this, component);
        return component;
}

template <typename T, typename... Args>
T &Entity::AddOrReplaceComponent(Args &&...args)
{
        IC_CORE_ASSERT(IsValid(), "Invalid entity!");
        T &component = scene->m_Registry.emplace_or_replace<T>(handle, std::forward<Args>(args)...);
        scene->OnComponentAdded<T>(*this, component);
        return component;
}

template <typename T>
T &Entity::GetComponent()
{
        IC_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        return scene->m_Registry.get<T>(handle);
}

template <typename T>
T *Entity::TryGetComponent()
{
        return scene->m_Registry.try_get<T>(handle);
}

template <typename T>
bool Entity::HasComponent() const
{
        return IsValid() && scene->m_Registry.all_of<T>(handle);
}

template <typename T>
void Entity::RemoveComponent()
{
        IC_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        scene->m_Registry.remove<T>(handle);
}

}  // namespace ic
#endif