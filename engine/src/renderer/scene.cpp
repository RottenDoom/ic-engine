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
	IC_CORE_WARN("Entity named {}, does not exist", name);
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

bool RenderScene::SetParent(Entity child, Entity parent) {
	// detach from old parent first
    	ClearParent(child);

    	auto& childHierarchy  = child.HasComponent<HierarchyComponent>()
                          ? child.GetComponent<HierarchyComponent>()
                          : child.AddComponent<HierarchyComponent>();

    	auto& parentHierarchy = parent.HasComponent<HierarchyComponent>()
                          ? parent.GetComponent<HierarchyComponent>()
                          : parent.AddComponent<HierarchyComponent>();

	childHierarchy.parent = parent;
	childHierarchy.depth  = parentHierarchy.depth + 1;
	parentHierarchy.children.push_back(child);
	return true;
}

bool RenderScene::ClearParent(Entity child) {
	if (!child.HasComponent<HierarchyComponent>()) 
		return false;
	auto& h = child.GetComponent<HierarchyComponent>();
	if (h.parent == entt::null)
		return false;

	// remove child from old parent's children list
	Entity oldParent(h.parent, this);
	if (oldParent.HasComponent<HierarchyComponent>())
	{
		auto& ph = oldParent.GetComponent<HierarchyComponent>();
		auto& c  = ph.children;
		c.erase(std::remove(c.begin(), c.end(), (entt::entity)child), c.end());
	}
	h.parent = entt::null;
	h.depth  = 0;
	return true;
}

glm::mat4 RenderScene::GetWorldTransform(Entity e)
{
	glm::mat4 local = e.HasComponent<TransformComponent>()
			? e.GetComponent<TransformComponent>().GetTransformMatrix()
			: glm::mat4(1.0f);

	if (!e.HasComponent<HierarchyComponent>()) return local;

	auto& h = e.GetComponent<HierarchyComponent>();
	if (h.parent == entt::null) return local;

	// recurse up the chain — parent world * local
	return GetWorldTransform(Entity(h.parent, this)) * local;
}

std::vector<Entity> RenderScene::GetAllEntities() {
	std::vector<Entity> result;
	for (auto handle : m_Registry.view<TagComponent>())
		result.emplace_back(handle, this);
	return result;
}

Entity RenderScene::GetParent(Entity e) 
{
	auto& h = e.HasComponent<HierarchyComponent>() ? e.GetComponent<HierarchyComponent>() : e.AddComponent<HierarchyComponent>();
	return {h.parent, this};
}

std::vector<Entity> RenderScene::GetChildren(Entity e)
{
	auto& h = e.HasComponent<HierarchyComponent>() ? e.GetComponent<HierarchyComponent>() : e.AddComponent<HierarchyComponent>();
	std::vector<Entity> res;

	for (auto e : h.children) {
		res.push_back({e, this});
	}

	return res;
}


std::vector<Entity> RenderScene::GetRoots()
{
	std::vector<Entity> roots;
    	// entities with no HierarchyComponent are also roots
    	auto allEntities = m_Registry.view<TagComponent>();
    	for (auto handle : allEntities)
    	{
        	Entity e(handle, this);
        	if (!e.HasComponent<HierarchyComponent>())
            		roots.emplace_back(e);
        	else if (e.GetComponent<HierarchyComponent>().parent == entt::null)
            		roots.emplace_back(e);
    	}
    	return roots;
}

bool RenderScene::HasEntity(UUID uuid) const {
	NOT_IMPL();
	return false;
}

}  // namespace ic
