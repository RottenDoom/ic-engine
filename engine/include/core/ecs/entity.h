#ifndef ENTITY_H
#define ENTITY_H

#include "defines.h"
#include "core/uuid.h"
#include "core/ecs/components.h"
#include <entt/entt.hpp>

namespace ic
{

class RenderScene;

class IC_API Entity
{
public:
        Entity() : handle(entt::null), scene(nullptr) {}
        Entity(entt::entity handle, RenderScene *scene);
        Entity(const Entity &other) = default;

        // declarations only — implementations in entity_impl.h
        template <typename T, typename... Args>
        T &AddComponent(Args &&...args);
        template <typename T, typename... Args>
        T &AddOrReplaceComponent(Args &&...args);
        template <typename T>
        T &GetComponent();
        template <typename T>
        T *TryGetComponent();
        template <typename T>
        bool HasComponent() const;
        template <typename T>
        void RemoveComponent();

        UUID               GetUUID() { return GetComponent<IDComponent>().ID; }
        const std::string &GetName() { return GetComponent<TagComponent>().Tag; }

        bool IsValid() const { return handle != entt::null && scene != nullptr; }

        operator bool() const { return IsValid(); }
        operator entt::entity() const { return handle; }
        operator uint32_t() const { return (uint32_t)handle; }

        bool operator==(const Entity &other) const { return handle == other.handle && scene == other.scene; }
        bool operator!=(const Entity &other) const { return !(*this == other); }

private:
        entt::entity handle;
        RenderScene *scene;
};

}  // namespace ic

#endif
