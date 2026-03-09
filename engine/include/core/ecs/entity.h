#ifndef ENTITY_H
#define ENTITY_H

#include "defines.h"
#include "renderer/camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ic
{
/**
 * Basic Sparse Entity component system
 * This is just a basic addition since I want to add a UI as soon as possible.
 * After writiing a basic configurable UI I will then go onto adding more stuff to the ECS while also improving
 * AssetManager. Since I want to follow KISS principle this will be a basic definition of ECS until the AssetManager
 * does not end up including MultiThreading loads and Fast file loads and caching
 *
 */

// Entity is just an ID for now
using Entity                           = uint32_t;
static constexpr Entity INVALID_ENTITY = 0;

// Interface class which I will design later
class Component;

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

#endif  // ENTITY_H