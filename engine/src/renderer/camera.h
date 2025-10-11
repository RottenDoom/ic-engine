#pragma once
#include "defines.h"

#include "core/events/event.h"
#include "core/events/mouse_event.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ic
{
        class Camera
        {
        public:
                float fovY, aspectRatio, znear, zfar;

                Camera() = default;
                Camera(float fov, float aspectRatio, float znear, float zfar);

                void onUpdate(float deltaTime);
                void onEvent(event& e);

                inline float getDistance() { return distance; }
                inline float setDistance(float distance) { this->distance = distance; }

                const glm::mat4& getViewMatrix() const { return m_viewMatrix; }
                const glm::mat4& getProjection() const { return m_projection; }
                glm::mat4 getViewProjection() const { return m_projection * m_viewMatrix; }

                glm::vec3 getUpDirection() const;
                glm::vec3 getRightDirection() const;
                glm::vec3 getForwardDirection() const;
                const glm::vec3& getPosition() const { return m_position; }
                glm::quat getOrientation() const;

                float getPitch() const { return pitch; }
                float getYaw() const { return yaw; }

        private:
                void updateProjection();
                void updateView();

                bool onMouseScroll(MouseScrolledEvent& e);

                void mousePan(const glm::vec2& delta);
                void mouseRotate(const glm::vec2& delta);
                void mouseZoom(float delta);

                glm::vec3 calculatePosition() const;

                glm::vec2 panSpeed() const;
                float rotationSpeed() const;
                float zoomSpeed() const;

        private:
                glm::mat4 m_viewMatrix;
                glm::mat4 m_projection = glm::mat4(1.0f);
                glm::vec3 m_position   = glm::vec3(0.0f);
                glm::vec3 m_focalPoint = glm::vec3(0.0f);

                glm::vec2 m_mousePos   = glm::vec2(0.0f);

                float distance         = 10.0f;
                float pitch = 0.0f, yaw = 0.0f;

                float viewportWidth, viewportHeight;
        };
}  // namespace ic
