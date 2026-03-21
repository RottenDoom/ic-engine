#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "defines.h"
#include "renderer/camera.h"
#include "core/assets/types/asset_base.h"
#include "core/uuid.h"

#include <entt/entt.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// namespace entt {
// enum class entity;
// }

namespace ic
{

struct IDComponent
{
        UUID ID;

        IDComponent()                    = default;
        IDComponent(const IDComponent &) = default;
        IDComponent(UUID uuid) : ID(uuid) {}
};

struct TagComponent
{
        std::string Tag;

        TagComponent()                     = default;
        TagComponent(const TagComponent &) = default;
        TagComponent(const std::string &tag) : Tag(tag) {}
};

struct HierarchyComponent
{
	entt::entity            parent   = entt::null;
	std::vector<entt::entity> children = {};
	uint32_t                 depth    = 0;  // root = 0, child = 1, etc.
};

struct TransformComponent
{
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity quaternion
        glm::vec3 scale    = glm::vec3(1.0f);

        mutable glm::mat4 transformMatrix = glm::mat4(1.0f);
        mutable bool      transformDirty  = true;

        void SetPosition(const glm::vec3 &pos)
        {
                position       = pos;
                transformDirty = true;
        }

        void SetRotation(const glm::quat &rot)
        {
                rotation       = rot;
                transformDirty = true;
        }

        void SetScale(const glm::vec3 &s)
        {
                scale          = s;
                transformDirty = true;
        }

        glm::mat4 GetTransformMatrix() const
        {
                if (transformDirty)
                {
                        // Calculate transformation matrix
                        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
                        glm::mat4 rotationMatrix    = glm::mat4_cast(rotation);
                        glm::mat4 scaleMatrix       = glm::scale(glm::mat4(1.0f), scale);

                        transformMatrix = translationMatrix * rotationMatrix * scaleMatrix;
                        transformDirty  = false;
                }
                return transformMatrix;
        }
};

struct MeshComponent
{
        IC_GUID modelID;

        // Material ID or material pointer
        void SetMesh(IC_GUID id) { modelID = id; }
        // void SetMaterial(MaterialID id) {MaterialID = id;}

        // Some kind of render component?
        // some render and update function?
};

struct CameraComponent
{
        Camera camera;
};

// TODO this maybe.
struct LightComponent
{
        glm::vec3 color{1};
        float     intensity = 1.0f;
};

}  // namespace ic

#endif  // COMPONENTS_H