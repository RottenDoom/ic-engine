#pragma once
#include "defines.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ic
{
        class Camera
        {
        private:
                float fov;  // radians
                float znear, zfar;

        public:
                /** @brief Camera Types for look at and firstperson view. A third person view might be used when needed
                 */
                enum CameraType
                {
                        lookAt,
                        firstPerson
                };

                CameraType type = CameraType::lookAt;

                /** @brief A camera has position its orientation ussually in quaternions etc */
                glm::vec3 rotation = glm::vec3();
                glm::vec3 position = glm::vec3();
                glm::vec4 viewPos  = glm::vec4();  // view position of the camera is

                /** @brief Camera speeds*/
                float rotationSpeed = 1.0f;
                float movementSpeed = 1.0f;

                bool updated        = true;
                bool flipY          = false;

                struct
                {
                        glm::mat4 perspective;
                        glm::mat4 view;
                } matrices;

                struct
                {
                        bool left  = false;  // A
                        bool right = false;  // D
                        bool up    = false;  // W
                        bool down  = false;  // s
                } keys;

                // functions for validation
                bool moving() const { return keys.left || keys.right || keys.up || keys.down; }
                float getNearClip() const { return znear; }
                float getFarClip() const { return zfar; }

                void
                setOrthographicProjection(float left, float right, float top, float bottom, float znear, float zfar);
                void setPerspectiveProjection(float fovY, float aspectRatio, float znear, float zfar);

                void setAspectRatio(float aspectRatio);

                void setPosition(glm::vec3 position)
                {
                        this->position = position;
                        updateViewMatrix();
                }

                void setRotation(glm::vec3 rotation)
                {
                        this->rotation = rotation;
                        updateViewMatrix();
                }

                void rotate(glm::vec3 delta)
                {
                        this->rotation += delta;
                        updateViewMatrix();
                }

                void translate(glm::vec3 delta) { this->position += delta; }

                void setRotationSpeed(float rotationSpeed) { this->rotationSpeed = rotationSpeed; }

                void setMovementSpeed(float movementSpeed) { this->movementSpeed = movementSpeed; }

                void update(float deltaTime);
                bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime);

                const glm::mat4 getProjectionMatrix() { return m_projectionMatrix; }

        private:
                glm::mat4 m_projectionMatrix;
                void updateViewMatrix();
        };
}  // namespace ic
