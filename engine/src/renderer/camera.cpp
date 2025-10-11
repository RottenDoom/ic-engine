#include "camera.h"

namespace ic
{
        void Camera::setOrthographicProjection(float left, float right, float top, float bottom, float znear, float zfar)
        {
        }

        void Camera::setPerspectiveProjection(float fovY, float aspectRatio, float znear, float zfar)
        {
                glm::mat4 curMatrix  = matrices.perspective;
                this->fov            = fovY;
                this->znear          = znear;
                this->zfar           = zfar;
                matrices.perspective = glm::perspective(glm::radians(fovY), aspectRatio, znear, zfar);

                if (flipY)
                {
                        matrices.perspective[1][1] *= -1.0f;
                }

                if (matrices.view != curMatrix)
                {
                        updated = true;
                }
        }

        void Camera::setAspectRatio(float aspectRatio)
        {
                glm::mat4 curMatrix  = matrices.perspective;
                matrices.perspective = glm::perspective(glm::radians(fov), aspectRatio, znear, zfar);
                if (flipY)
                        matrices.perspective[1][1] *= -1.0f;
                if (matrices.view != curMatrix)
                        updated = true;
        }

        void Camera::update(float deltaTime)
        {
                // first time using delta time in this engine
                updated = false;
                if (type = CameraType::firstPerson)
                {
                        if (moving())
                        {
                                glm::vec3 camPos;
                                camPos.x        = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
                                camPos.y        = sin(glm::radians(rotation.x));
                                camPos.z        = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
                                camPos          = glm::normalize(camPos);

                                float moveSpeed = deltaTime * movementSpeed;

                                if (keys.up)
                                        position += camPos * moveSpeed;
                                if (keys.down)
                                        position -= camPos * moveSpeed;
                                if (keys.left)
                                        position -= glm::normalize(glm::cross(camPos, glm::vec3(0.0f, 1.0f, 0.0f))) *
                                                    moveSpeed;
                                if (keys.down)
                                        position += glm::normalize(glm::cross(camPos, glm::vec3(0.0f, 1.0f, 0.0f))) *
                                                    moveSpeed;
                        }
                }
                updateViewMatrix();
        }

        bool Camera::updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
        {
                // for gamepad controls.
                return false;
        }

        void Camera::updateViewMatrix()
        {
                glm::mat4 curMatrix = matrices.view;

                glm::mat4 Mrot      = glm::mat4(1.0f);
                glm::mat4 Mtrans;

                Mrot                  = glm::rotate(Mrot,
                                   glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)),
                                   glm::vec3(1.0f, 0.0f, 0.0f));
                Mrot                  = glm::rotate(Mrot, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                Mrot                  = glm::rotate(Mrot, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

                glm::vec3 translation = position;
                if (flipY)
                {
                        translation.y *= -1.0f;
                }
                Mtrans = glm::translate(glm::mat4(1.0f), translation);

                if (type == CameraType::firstPerson)
                {
                        matrices.view = Mrot * Mtrans;
                }
                else
                {
                        matrices.view = Mtrans * Mrot;
                }

                viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

                if (matrices.view != curMatrix)
                {
                        updated = true;
                }
        }

}  // namespace ic
