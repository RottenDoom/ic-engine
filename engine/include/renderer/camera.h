#ifndef CAMERA_H
#define CAMERA_H
#include "defines.h"

#include "renderer/camera.h"
#include "core/events/event.h"
#include "core/events/key_event.h"
#include "core/events/mouse_event.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class IC_API Camera
{
private:
        void UpdateViewMatrix();

        void ResetCameraPosition();
        void HandleInput(float deltaTime);

public:
        float fovY;
        float znear, zfar;
        float aspectRatio;

        enum CameraType
        {
                lookat,
                firstperson
        };
        CameraType type = CameraType::lookat;

        // camera attributes
        glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 position    = glm::vec3(0.0f);
        bool      flipY       = false;

        // the projection matrix used for projecting onto the screen into the clip space.
        glm::mat4 projection = glm::mat4(1.0f);
        struct
        {
                glm::mat4 perspective;
                glm::mat4 view;
                glm::mat4 orthographic;
        } matrices;

        float zoomSpeed        = 0.5f;
        float mouseSensitivity = 0.5f;
        float movementSpeed    = 10.0f;  // bumped up the speed as debug camera is too slow

        bool updated    = true;
        bool fovChanged = false;

        struct
        {
                bool left  = false;
                bool right = false;
                bool up    = false;
                bool down  = false;
        } keys;

        bool Moving() const { return keys.left || keys.right || keys.up || keys.down; }

        float GetNearClip() const { return znear; }
        float GetFarClip() const { return zfar; }

        // sets up perspective or orthographic projection
        void SetPerspectiveProjection(float fov, float aspect, float znear, float zfar);
        void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

        // set view direction set view target

        /// @brief This function takes in position and direction of the object we want to see and transforms
        /// into the eye coordinates using the view matrix more precisely its the inverse of rotation multiplied
        /// to the translation transform matrix. I am currently using quaternions since I am interested in them
        /// but will later use euler angles as well if needed and if they are simpler with more constraints
        /// @param position The position of the object the tranform will be negative of this
        /// @param direction The direction of the matrix we will calculate the quaternion to find the rotation
        /// matrix to that direction in the space and multiply the negative of it to the tranform to find the
        /// view matrix
        void SetViewDirection(glm::vec3 position, glm::vec3 direction);
        void SetViewTarget(glm::vec3 position, glm::vec3 target);

        void UpdateAspectRatio(float aspect);

        void SetPosition(glm::vec3 position)
        {
                this->position = position;
                UpdateViewMatrix();
        }

        void SetTranslation(glm::vec3 translation)
        {
                this->position = translation;
                UpdateViewMatrix();
        };

        // same as set direction except it sets the quaternion of the camera direction
        void SetOrientation(glm::vec3 angles)
        {
                this->orientation = glm::quat(angles);
                UpdateViewMatrix();
        }

        void Translate(glm::vec3 delta)
        {
                this->position += delta;
                UpdateViewMatrix();
        }

        void SetViewPortSize(uint32_t width, uint32_t height)
        {
                if (height != 0)
                {
                        float aspect = (float)width / (float)height;
                        UpdateAspectRatio(aspect);
                }
                else
                {
                        // [TODO] Handle this better
                        IC_CORE_INFO("The application is minimized or doesn't exist");
                }
        }

        void SetZoomSpeed(float speed) { this->zoomSpeed = speed; }
        void SetMouseSensitivity(float sensitivity) { this->mouseSensitivity = sensitivity; }
        void SetMovementSpeed(float speed) { this->movementSpeed = speed; }

        void OnUpdate(float deltaTime);
        void OnEvent(ic::event &e);
        bool OnKeyPressed(ic::KeyPressedEvent &e);
        bool OnMouseMoved(ic::MouseMovedEvent &e);
        bool OnMouseScroll(ic::MouseScrolledEvent &e);
};

#ifdef __cplusplus
extern "C"
{
#endif

        /**
         * @function createCamera
         * @category rendering
         * @brief Create a 3D camera. 2D camera is to extended for
         * @param ic::Camera::CameraType type - can lookat or firstperson
         * @param float* position array of size 3
         * @param float* orientation array of size 4 (quaternion)
         */
        IC_API Camera createCamera(Camera::CameraType type, glm::vec3 position);

#ifdef __cplusplus
}
#endif

#endif