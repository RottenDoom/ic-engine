#include "renderer/camera.h"

#include "core/input.h"
#include "core/keycodes.h"
#include "core/mousecodes.h"

// TODO: remove
#include "core/application.h"

#include <glm/gtx/quaternion.hpp>

/**
 * TODO panning motion and turning of certain motions
 * Ofcourse adding somethings to the UI to control movements
 */

#define degreeToRadian(x) (x * (1 / 57.295779513082320876798154814105))

void Camera::updateViewMatrix()
{
        glm::mat4 rotation    = glm::mat4_cast(glm::conjugate(orientation));
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), -position);
        matrices.view         = rotation * translation;
}

void Camera::setPerspectiveProjection(float fov, float aspect, float znear, float zfar)
{
        this->zfar        = zfar;
        this->znear       = znear;
        this->fovY        = fov;
        this->aspectRatio = aspect;

        IC_CORE_ASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f, "Aspect Ratio Error!");
        const float tanHalfFovy = tan(degreeToRadian(fov) / 2.f);
        projection              = glm::mat4{0.0f};
        projection[0][0]        = 1.0f / (aspect * tanHalfFovy);
        projection[1][1]        = 1.0f / (tanHalfFovy);
        projection[2][2]        = zfar / (zfar - znear);
        projection[2][3]        = 1.0f;
        projection[3][2]        = -(zfar * znear) / (zfar - znear);

        if (flipY)
                projection[1][1] *= -1;
}

void Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far)
{
        this->zfar       = far;
        this->znear      = near;

        projection       = glm::mat4(1.0f);
        projection[0][0] = 2.0f / (right - left);
        projection[1][1] = 2.0f / (bottom - top);
        projection[2][2] = 1.0f / (far - near);
        projection[3][0] = -(right + left) / (right - left);
        projection[3][1] = -(bottom + top) / (bottom - top);
        projection[3][2] = -near / (far - near);

        if (flipY)
                projection[1][1] *= -1;
}

void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction)
{
        glm::vec3 forward = glm::normalize(direction);
        glm::vec3 right   = glm::normalize(glm::cross(glm::vec3(0.0f, (flipY ? -1.0f : 1.0f), 0.0f), forward));
        glm::vec3 up      = glm::cross(forward, right);

        glm::mat3 rot(right, up, -forward);
        this->orientation = glm::normalize(glm::quat_cast(rot));
        this->position    = position;

        // FIX THIS
        updateViewMatrix();
}

void Camera::setViewTarget(glm::vec3 position, glm::vec3 target)
{
        if (glm::normalize(target - position) == glm::vec3(0.0f))
        {
                IC_CORE_ERROR("Camera: position and target point to the same location!");
                return;
        }
        setViewDirection(position, target - position);
}

void Camera::updateAspectRatio(float aspect)
{
        glm::mat4 currentMatrix = matrices.perspective;
        matrices.perspective    = glm::perspective(glm::radians(fovY), aspect, znear, zfar);
        if (flipY)
        {
                matrices.perspective[1][1] *= -1.0f;
        }
        if (matrices.view != currentMatrix)
        {
                updated = true;
        }
}

void Camera::resetCameraPosition()
{
        // Reset position to world origin or a desired point
        position = glm::vec3(0.0f, 0.0f, -10.0f);  // Camera 10 units back, facing origin

        // Reset orientation to look toward -Z (default forward direction)
        orientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));  // Identity rotation

        // Reset FOV and mark that projection may need update
        fovY       = 45.0f;
        fovChanged = true;

        // Recalculate the view matrix based on new transforms
        updateViewMatrix();

        updated = true;
}

void Camera::handleInput(float deltaTime)
{
        if (type == firstperson)
        {
                float velocity    = movementSpeed * deltaTime;

                glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
                glm::vec3 right   = orientation * glm::vec3(1.0f, 0.0f, 0.0f);
                glm::vec3 up      = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

                if (ic::input::isKeyPressed(ic::Key::W))
                        position += forward * velocity;
                if (ic::input::isKeyPressed(ic::Key::S))
                        position -= forward * velocity;
                if (ic::input::isKeyPressed(ic::Key::A))
                        position -= right * velocity;
                if (ic::input::isKeyPressed(ic::Key::D))
                        position += right * velocity;
                if (ic::input::isKeyPressed(ic::Key::Space))
                        position += up * velocity;
                if (ic::input::isKeyPressed(ic::Key::LeftShift))
                        position -= up * velocity;
        }
}

void Camera::onUpdate(float deltaTime)
{
        updated = false;
        handleInput(deltaTime);
        updateViewMatrix();

        if (fovChanged)
                setPerspectiveProjection(this->fovY, this->aspectRatio, this->znear, this->zfar);
        fovChanged = false;
}

void Camera::onEvent(ic::event &e)
{
        ic::eventDispatcher dispatcher(e);
        dispatcher.dispatch<ic::MouseMovedEvent>(BIND_EVENT(Camera::onMouseMoved));
        dispatcher.dispatch<ic::MouseScrolledEvent>(BIND_EVENT(Camera::onMouseScroll));
        dispatcher.dispatch<ic::KeyPressedEvent>(BIND_EVENT(Camera::onKeyPressed));
}

bool Camera::onKeyPressed(ic::KeyPressedEvent &e)
{
        if (ic::input::isKeyPressed(ic::Key::R))
        {
                resetCameraPosition();
        }
        if (ic::input::isKeyPressed(ic::Key::C))
        {
                switch (type)
                {
                case lookat:
                        type = firstperson;
                case firstperson:
                        type = lookat;
                }
        }

        return false;
}

bool Camera::onMouseMoved(ic::MouseMovedEvent &e)
{
        static bool firstMouse = true;
        static float lastX     = 0.0f;
        static float lastY     = 0.0f;

        float xpos             = e.getX();
        float ypos             = e.getY();

        if (firstMouse)
        {
                lastX      = xpos;
                lastY      = ypos;
                firstMouse = false;
        }

        // Compute delta movement
        float xoffset = -(xpos - lastX);
        float yoffset = lastY - ypos;  // reversed: moving up should look up

        lastX         = xpos;
        lastY         = ypos;

        // Apply sensitivity
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        // Convert deltas to quaternion rotations
        glm::quat yaw   = glm::angleAxis(glm::radians(-xoffset), glm::vec3(0, 1, 0));
        glm::quat pitch = glm::angleAxis(glm::radians(-yoffset), glm::vec3(1, 0, 0));

        // Combine rotations: yaw first, then pitch
        orientation = glm::normalize(yaw * orientation * pitch);

        updateViewMatrix();
        return false;
}

bool Camera::onMouseScroll(ic::MouseScrolledEvent &e)
{
        fovY       -= e.getYOffset() * zoomSpeed;      // zoom speed
        fovY        = glm::clamp(fovY, 10.0f, 90.0f);  // prevent extreme zoom
        fovChanged  = true;
        return false;
}

Camera createCamera(Camera::CameraType type, glm::vec3 position)
{
        Camera camera;

        camera.type = Camera::CameraType::firstperson;
        camera.setViewDirection(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        camera.setPerspectiveProjection(45.0f,
                                        (float)ic::Application::get().getWindow().getWidth() /
                                            (float)ic::Application::get().getWindow().getHeight(),
                                        0.1f,
                                        256.0f);
        return camera;
}