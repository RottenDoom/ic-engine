#include "renderer/scene.h"
#include "core/ecs/entity.h"
#include "core/ecs/entity_impl.h"

#include <entt/entt.hpp>

namespace ic
{
RenderScene::RenderScene() {}

RenderScene::~RenderScene() {}

Entity RenderScene::CreateEntityWithName(const string &name)
{
        return CreateEntity(UUID(), name);
}

Entity RenderScene::CreateEntity(UUID uuid, const string &name)
{
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        TagComponent &tag = entity.AddComponent<TagComponent>();
        tag.Tag           = name.empty() ? "Entity" : name;

        m_EntityMap[uuid] = entity;
        return entity;
}

void RenderScene::DestroyEntity(Entity entity)
{
        m_EntityMap.erase(entity.GetUUID());
}

Entity RenderScene::FindEntityByName(std::string_view name)
{
        auto view = m_Registry.view<TagComponent>();
        for (auto entity : view)
        {
                const TagComponent &tc = view.get<TagComponent>(entity);
                if (tc.Tag == name)
                        return Entity{entity, this};
        }
        return NULL_ENTITY;
}

Entity RenderScene::GetEntityByUUID(UUID uuid)
{
        if (m_EntityMap.find(uuid) != m_EntityMap.end())
        {
                return {m_EntityMap.at(uuid), this};
        }
        return NULL_ENTITY;
}

template <>
void RenderScene::OnComponentAdded<IDComponent>(Entity entity, IDComponent &component)
{
}

template <>
void RenderScene::OnComponentAdded<TagComponent>(Entity entity, TagComponent &component)
{
}

template <>
void RenderScene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent &component)
{
}

template <>
void RenderScene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent &component)
{
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
                component.camera.setViewPortSize(m_ViewportWidth, m_ViewportHeight);
}

template <>
void RenderScene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent &component)
{
}

template <>
void RenderScene::OnComponentAdded<LightComponent>(Entity entity, LightComponent &component)
{
}

}  // namespace ic
