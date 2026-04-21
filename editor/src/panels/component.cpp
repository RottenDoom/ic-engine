#include "panels.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace ic::panels
{

// Helper: draw a labeled vec3 drag — cleaner than raw DragFloat3
static bool draw_vec3(const char *label, glm::vec3 &v, float speed = 0.1f, const char *fmt = "%.3f")
{
        ImGui::PushID(label);
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 90.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        bool changed = ImGui::DragFloat3("##v", glm::value_ptr(v), speed, 0.0f, 0.0f, fmt);
        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
}

void component_panel_draw(ic::Entity &selected)
{
        ImGui::Begin("Components");

        if (!selected.IsValid())
        {
                ImGui::TextDisabled("No entity selected");
                ImGui::End();
                return;
        }

        // ---- Entity name ----
        {
                std::string name = selected.GetName();
                char        buf[256];
                strncpy(buf, name.c_str(), sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = '\0';

                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##name", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                        // TODO: expose RenameEntity on RenderScene
                        // selected.GetComponent<ic::TagComponent>().Tag = buf;
                }
        }

        ImGui::Separator();

        // ---- TransformComponent ----
        if (selected.HasComponent<ic::TransformComponent>())
        {
                bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);

                if (open)
                {
                        auto &t = selected.GetComponent<ic::TransformComponent>();

                        glm::vec3 pos = t.position;
                        if (draw_vec3("Position", pos, 0.1f))
                                t.SetPosition(pos);

                        // Convert quat to euler degrees for display, back to quat on edit
                        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(t.rotation));
                        if (draw_vec3("Rotation", eulerDeg, 1.0f, "%.1f"))
                        {
                                t.SetRotation(glm::quat(glm::radians(eulerDeg)));
                        }

                        glm::vec3 scl = t.scale;
                        if (draw_vec3("Scale", scl, 0.01f))
                                t.SetScale(scl);
                }
        }

        // ---- MeshComponent ----
        if (selected.HasComponent<ic::MeshComponent>())
        {
                if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
                {
                        auto       &m         = selected.GetComponent<ic::MeshComponent>();
                        std::string modelName = AssetManager::Get().GetRegistry()->GetAssetName(
                            m.modelID);  // ISSUE: If the mesh id does not exist load it from some location.
                        ImGui::LabelText("Model", "%s", modelName.empty() ? "(none)" : modelName.c_str());

                        // TODO: drag-drop from asset browser to change model
                }
        }

        // ---- LightComponent ----
        if (selected.HasComponent<ic::LightComponent>())
        {
                if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
                {
                        auto &lc = selected.GetComponent<ic::LightComponent>();
                        light_panel_draw(lc);
                }
        }

        // ---- Add Component button ----
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Button("Add Component", ImVec2(-1, 0)))
        {
                ImGui::OpenPopup("##AddComponent");
        }

        if (ImGui::BeginPopup("##AddComponent"))
        {
                if (!selected.HasComponent<ic::MeshComponent>())
                {
                        if (ImGui::MenuItem("Mesh Component"))
                        {

                                selected.AddComponent<ic::MeshComponent>();
                        }
                }
                if (!selected.HasComponent<ic::LightComponent>())
                {
                        if (ImGui::MenuItem("Light Component"))
                        {
                                selected.AddComponent<ic::LightComponent>();
                        }
                }
                ImGui::EndPopup();
        }

        ImGui::End();
}

}  // namespace ic::panels
