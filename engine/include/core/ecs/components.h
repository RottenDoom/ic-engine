#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "defines.h"
#include "renderer/camera.h"
#include "core/uuid.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ic
{

struct IDComponent
{
        UUID ID;

        IDComponent()                    = default;
        IDComponent(const IDComponent &) = default;
};

struct TagComponent
{
        std::string Tag;

        TagComponent()                     = default;
        TagComponent(const TagComponent &) = default;
        TagComponent(const std::string &tag) : Tag(tag) {}
};

struct TransformComponent
{
        glm::vec3 position{0};
        glm::quat rotation{1, 0, 0, 0};
        glm::vec3 scale{1};

        glm::mat4 matrix() const
        {
                glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
                glm::mat4 R = glm::mat4_cast(rotation);
                glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
                return T * R * S;
        }
};

struct MeshComponent
{
        GUID modelID;
};

struct CameraComponent
{
        Camera camera;
};

struct LightComponent
{
        glm::vec3 color{1};
        float     intensity = 1.0f;
};

}  // namespace ic

#endif  // COMPONENTS_H