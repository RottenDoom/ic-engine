#include "camera.h"

#include "core/input.h"
#include "core/keycodes.h"
#include "core/mousecodes.h"

#include <glm/gtx/quaternion.hpp>

namespace ic
{
        Camera::Camera(float fov, float aspectRatio, float znear, float zfar)
            : fovY(fov), aspectRatio(aspectRatio), znear(znear), zfar(zfar)
        {
                updateView();
        }

        void Camera::updateProjection()
        {
                aspectRatio  = viewportWidth / viewportHeight;  // ?? fix tis
                m_projection = glm::perspective(glm::radians(fovY), aspectRatio, znear, zfar);
        }

        void Camera::updateView()
        {
                m_position            = calculatePosition();

                glm::quat orientation = getOrientation();
                m_viewMatrix          = glm::translate(glm::mat4(1.0f), m_position) * glm::toMat4(orientation);
                m_viewMatrix          = glm::inverse(m_viewMatrix);
        }

}  // namespace ic
