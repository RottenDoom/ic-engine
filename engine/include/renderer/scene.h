#ifndef SCENE_H
#define SCENE_H

#include "defines.h"
#include "camera.h"
#include "core/assets/types/asset_base.h"
#include "core/ecs/entity.h"
#include "core/application.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/** Scene class that can be setup by user or anyone. */
namespace ic
{
class IC_API RenderScene
{
public:
        Entity createEntity()
        {
                Entity e = ++m_next;
                m_entities.push_back(e);
                return e;
        }

        void destroyEntity(Entity e)
        {
                // simplified for now
        }

        Camera camera;

        TransformComponent &addTransform(Entity e) { return m_transforms[e]; }

        MeshComponent &addMesh(Entity e) { return m_meshes[e]; }

        CameraComponent &addCamera(Entity e) { return m_cameras[e]; }

        std::unordered_map<Entity, TransformComponent> &transforms() { return m_transforms; }
        std::unordered_map<Entity, MeshComponent>      &meshes() { return m_meshes; }
        std::unordered_map<Entity, CameraComponent>    &cameras() { return m_cameras; }

private:
        Entity m_next = 0;

        std::vector<Entity> m_entities;

        std::unordered_map<Entity, TransformComponent> m_transforms;
        std::unordered_map<Entity, MeshComponent>      m_meshes;
        std::unordered_map<Entity, CameraComponent>    m_cameras;
};

}  // namespace ic

#endif