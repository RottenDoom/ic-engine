#include "panels.h"
#include <renderer/camera.h>
#include <imgui.h>

static constexpr float minZoomSpeed = 0.1f;
static constexpr float maxZoomSpeed = 5.0f;

static constexpr float minSpeed = 1.0f;
static constexpr float maxSpeed = 10.0f;

static constexpr float minMouseSensitivity = 0.1f;
static constexpr float maxMouseSensitivity = 1.0f;

namespace ic::panels
{

void camera_panel_draw(Camera &editorCamera)
{

        // TODO:
        /**
         * 3. Hide camera from scene.
         * 5. Change FOv, aspect ratio, near and far plane.
         * 6. Perspective or Orthographic
         * 7. Can lock targets or entity references
         * 8. Culling
         * 9. Post processing bs
         * 10 Make a camera component controller after this
         */

        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
                // Camera type toggle
                int type = (int)editorCamera.type;
                ImGui::RadioButton("First Person", &type, (int)Camera::CameraType::firstperson);
                ImGui::SameLine();
                ImGui::RadioButton("Look At", &type, (int)Camera::CameraType::lookat);
                editorCamera.type = (Camera::CameraType)type;

                // Translation (replace this with components)
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                        glm::vec3 pos = editorCamera.position;
                        if (draw_vec3("Position", pos, 0.1f))
                                editorCamera.SetPosition(pos);

                        // Convert quat to euler degrees for display, back to quat on edit
                        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(editorCamera.orientation));
                        if (draw_vec3("Rotation", eulerDeg, 1.0f, "%.1f"))
                        {
                                editorCamera.SetOrientation(eulerDeg);
                        }
                }

                // Sensitivity speed and zoom
                ImGui::DragScalar(
                    "Zoom Speed", ImGuiDataType_Float, &editorCamera.zoomSpeed, 0.05f, &minZoomSpeed, &maxZoomSpeed);
                ImGui::DragScalar(
                    "Speed", ImGuiDataType_Float, &editorCamera.movementSpeed, 0.05f, &minSpeed, &maxSpeed);
                ImGui::DragScalar("Mouse Sensitivity",
                                  ImGuiDataType_Float,
                                  &editorCamera.mouseSensitivity,
                                  0.05f,
                                  &minMouseSensitivity,
                                  &maxMouseSensitivity);
        }
}
}  // namespace ic::panels