#include "panels.h"
#include "editor.h"
#include <imgui.h>

// TODO:
// Fix the camera component so that when hovering over the panels the scene does not get updated. Also in the camera
// make a toggle that works when I press c etc.
// Editor camera and other cameras (Editor camera is the priority now.`)
// New entity add more mesh does not work gives excpetion.
static uint8_t count_lights(ic::RenderScene *scene)
{
        uint8_t count = 0;
        scene->Each<ic::LightComponent>([&](auto) { count++; });
        return count;
}

namespace ic
{

void EditorSystem::DrawSceneHeirarchy()
{
        ImGui::Begin("Hierarchy");

        float availHeight     = ImGui::GetContentRegionAvail().y;
        float hierarchyHeight = availHeight - m_State.lastCameraPanelHeight;
        if (hierarchyHeight < 0.0f)
                hierarchyHeight = 0.0f;

        ImGui::BeginChild("SceneHierarchyRegion", ImVec2(0, hierarchyHeight), false);

        if (m_ActiveScene)
        {

                ImGui::PushID("SceneRoot");

                ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth |
                                               ImGuiTreeNodeFlags_OpenOnArrow;
                bool sceneOpen;

                if (m_State.renamingScene)
                {
                        sceneOpen = ImGui::TreeNodeEx("##scenenode", rootFlags | ImGuiTreeNodeFlags_Leaf, "");
                        ImGui::SameLine();
                        if (m_State.isFocused)
                        {
                                ImGui::SetKeyboardFocusHere();
                                m_State.isFocused = false;
                        }

                        ImGui::SetNextItemWidth(160.0f);
                        if (ImGui::InputText("##scenerename",
                                             m_State.renameBuffer,
                                             sizeof(m_State.renameBuffer),
                                             ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                                m_ActiveScene->SetName(m_State.renameBuffer);
                                m_State.renamingScene = false;
                        }

                        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
                        {
                                m_ActiveScene->SetName(m_State.renameBuffer);
                                m_State.renamingScene = false;
                        }
                }
                else
                {
                        sceneOpen = ImGui::TreeNodeEx("##scenenode", rootFlags, "%s", m_ActiveScene->GetName().c_str());
                        if (ImGui::BeginPopupContextItem("##scenectx"))
                        {
                                if (ImGui::MenuItem("Rename"))
                                {
                                        m_State.renamingScene = true;
                                        strncpy(m_State.renameBuffer,
                                                m_ActiveScene->GetName().c_str(),
                                                sizeof(m_State.renameBuffer));
                                        m_State.renameBuffer[sizeof(m_State.renameBuffer) - 1] = '\0';
                                        m_State.isFocused                                      = true;
                                }
                                if (ImGui::MenuItem("Create Empty Entity"))
                                        m_ActiveScene->CreateEntityWithName("Entity");

                                uint8_t n = count_lights(m_ActiveScene);
                                if (ImGui::BeginMenu("Create Lighting"))
                                {
                                        if (ImGui::MenuItem("Point Light"))
                                        {
                                                auto  entity = m_ActiveScene->CreateEntityWithName("PointLight_" +
                                                                                                  std::to_string(n));
                                                auto &lc     = entity.AddComponent<ic::LightComponent>();
                                                lc.type      = ic::LightType::Point;
                                                lc.color     = glm::vec3(1.0f);
                                                lc.intensity = 1.0f;
                                                lc.range     = 10.0f;
                                        }
                                        if (ImGui::MenuItem("Directional Light"))
                                        {
                                                auto  entity = m_ActiveScene->CreateEntityWithName("DirectionalLight_" +
                                                                                                  std::to_string(n));
                                                auto &lc     = entity.AddComponent<ic::LightComponent>();
                                                lc.type      = ic::LightType::Directional;
                                                lc.color     = glm::vec3(1.0f, 0.95f, 0.85f);
                                                lc.intensity = 1.0f;
                                        }
                                        if (ImGui::MenuItem("Spot Light"))
                                        {
                                                auto  entity  = m_ActiveScene->CreateEntityWithName("SpotLight_" +
                                                                                                  std::to_string(n));
                                                auto &lc      = entity.AddComponent<ic::LightComponent>();
                                                lc.type       = ic::LightType::Spot;
                                                lc.color      = glm::vec3(1.0f);
                                                lc.intensity  = 1.0f;
                                                lc.range      = 20.0f;
                                                lc.innerAngle = 12.5f;
                                                lc.outerAngle = 25.0f;
                                        }
                                        ImGui::EndMenu();
                                }
                                ImGui::EndPopup();
                        }
                }

                if (sceneOpen)
                {
                        std::vector<ic::Entity> entities = m_ActiveScene->GetAllEntities();

                        for (auto &e : entities)
                        {
                                if (!e.IsValid())
                                        continue;

                                std::string label = e.GetName();
                                if (label.empty())
                                        label = "(unnamed)";

                                bool isSelected = (m_State.selected && m_State.selected == e);

                                ImGui::PushID((int)e.GetUUID());

                                if (m_State.renameTarget == e)
                                {
                                        // Render a leaf node with empty label then put input beside it
                                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                                                   ImGuiTreeNodeFlags_SpanFullWidth |
                                                                   ImGuiTreeNodeFlags_Selected;
                                        ImGui::TreeNodeEx("##node", flags, "");
                                        ImGui::TreePop();

                                        ImGui::SameLine();

                                        if (m_State.isFocused)
                                        {
                                                ImGui::SetKeyboardFocusHere();
                                                m_State.isFocused = false;
                                        }
                                        ImGui::SetNextItemWidth(160.0f);
                                        if (ImGui::InputText("##rename",
                                                             m_State.renameBuffer,
                                                             sizeof(m_State.renameBuffer),
                                                             ImGuiInputTextFlags_AutoSelectAll |
                                                                 ImGuiInputTextFlags_EnterReturnsTrue))
                                        {
                                                e.SetName(m_State.renameBuffer);
                                                m_State.renameTarget = {};
                                        }
                                        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
                                        {
                                                e.SetName(m_State.renameBuffer);
                                                m_State.renameTarget = {};
                                        }
                                }
                                else
                                {
                                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                                                   ImGuiTreeNodeFlags_SpanFullWidth |
                                                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                                                   (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                                        ImGui::TreeNodeEx(label.c_str(), flags);
                                        ImGui::TreePop();

                                        if (ImGui::IsItemClicked())
                                                m_State.selected = e;

                                        if (ImGui::BeginPopupContextItem("##ctx"))
                                        {
                                                if (ImGui::MenuItem("Rename"))
                                                {
                                                        m_State.renameTarget = e;
                                                        strncpy(m_State.renameBuffer,
                                                                e.GetName().c_str(),
                                                                sizeof(m_State.renameBuffer));
                                                        m_State.renameBuffer[sizeof(m_State.renameBuffer) - 1] = '\0';
                                                        m_State.isFocused                                      = true;
                                                }
                                                if (ImGui::MenuItem("Duplicate"))
                                                {
                                                        ic::Entity dup = m_ActiveScene->CreateEntityWithName(
                                                            e.GetName() + "_copy");
                                                        if (e.HasComponent<ic::TransformComponent>())
                                                        {
                                                                auto &src = e.GetComponent<ic::TransformComponent>();
                                                                auto &dst = dup.GetComponent<ic::TransformComponent>();
                                                                dst.SetPosition(src.position);
                                                                dst.SetRotation(src.rotation);
                                                                dst.SetScale(src.scale);
                                                        }
                                                        if (e.HasComponent<ic::MeshComponent>())
                                                        {
                                                                auto &src = e.GetComponent<ic::MeshComponent>();
                                                                dup.AddComponent<ic::MeshComponent>().SetMesh(
                                                                    src.modelID);
                                                        }
                                                        m_State.selected = dup;
                                                }
                                                ImGui::Separator();
                                                if (ImGui::MenuItem("Delete"))
                                                {
                                                        m_ActiveScene->DestroyEntity(e);
                                                        if (m_State.selected && m_State.selected == e)
                                                                m_State.selected = {};
                                                }
                                                ImGui::EndPopup();
                                        }
                                }

                                ImGui::PopID();
                        }

                        ImGui::TreePop();
                }

                ImGui::PopID();  // SceneRoot
        }

        ImGui::EndChild();

        // really cool trick i guess
        float cameraStartY = ImGui::GetCursorPosY();

        ImGui::Separator();

        ic::panels::camera_panel_draw(m_EditorCamera);
        float cameraEndY = ImGui::GetCursorPosY();

        m_State.lastCameraPanelHeight = cameraEndY - cameraStartY;

        ImGui::End();
}

}  // namespace ic
