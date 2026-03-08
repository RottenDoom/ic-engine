#ifndef SCENE_H
#define SCENE_H

#include "defines.h"
#include "camera.h"
#include "core/assets/types/asset_base.h"
#include "core/application.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct RenderNode
{
        GUID modelID;
        glm::mat4 transform;
};

// See if this can be a struct
class IC_API Scene
{
public:
        Camera camera;
        std::vector<RenderNode> nodes;

        void AddModel(GUID id, glm::mat4 transform)
        {
                IC_CORE_TRACE("Added model ID = {} to the scene!", id);
                nodes.push_back({id, transform});
        }

        /** TODO: IMPORTANT: Fix this and better camera setup */
        void SetupCamera() { camera = createCamera(Camera::CameraType::firstperson, glm::vec3(1.0f)); }

        void Clear() { nodes.clear(); }
};

#endif