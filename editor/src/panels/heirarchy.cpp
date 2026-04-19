#include "panels.h"
#include "editor.h"
#include <imgui.h>

// I don't know if I should connect this with the rendergraph ds

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

namespace ic::panels
{

/** TODO: make this function not so big */
void heirarchy_draw(ic::RenderScene *scene, ic::EditorState &state)
{
        ImGui::Begin("Hierarchy");

        if (!scene)
        {
                ImGui::End();
                return;
        }

        ImGui::PushID("SceneRoot");

        ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth |
                                       ImGuiTreeNodeFlags_OpenOnArrow;
        bool sceneOpen;

        if (state.renamingScene)
        {
                sceneOpen = ImGui::TreeNodeEx("##scenenode", rootFlags | ImGuiTreeNodeFlags_Leaf, "");
                ImGui::SameLine();
                if (state.isFocused)
                {
                        ImGui::SetKeyboardFocusHere();
                        state.isFocused = false;
                }

                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::InputText("##scenerename",
                                     state.renameBuffer,
                                     sizeof(state.renameBuffer),
                                     ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                {
                        scene->SetName(state.renameBuffer);
                        state.renamingScene = false;
                }

                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
                {
                        scene->SetName(state.renameBuffer);
                        state.renamingScene = false;
                }
        }
        else
        {
                sceneOpen = ImGui::TreeNodeEx("##scenenode", rootFlags, "%s", scene->GetName().c_str());
                if (ImGui::BeginPopupContextItem("##scenectx"))
                {
                        if (ImGui::MenuItem("Rename"))
                        {
                                state.renamingScene = true;
                                strncpy(state.renameBuffer, scene->GetName().c_str(), sizeof(state.renameBuffer));
                                state.renameBuffer[sizeof(state.renameBuffer) - 1] = '\0';
                                state.isFocused                                    = true;
                        }
                        if (ImGui::MenuItem("Create Empty Entity"))
                                scene->CreateEntityWithName("Entity");

                        uint8_t n = count_lights(scene);
                        if (ImGui::BeginMenu("Create Lighting"))
                        {
                                if (ImGui::MenuItem("Point Light"))
                                {
                                        auto  entity = scene->CreateEntityWithName("PointLight_" + std::to_string(n));
                                        auto &lc     = entity.AddComponent<ic::LightComponent>();
                                        lc.type      = ic::LightType::Point;
                                        lc.color     = glm::vec3(1.0f);
                                        lc.intensity = 1.0f;
                                        lc.range     = 10.0f;
                                }
                                if (ImGui::MenuItem("Directional Light"))
                                {
                                        auto  entity = scene->CreateEntityWithName("DirectionalLight_" +
                                                                                  std::to_string(n));
                                        auto &lc     = entity.AddComponent<ic::LightComponent>();
                                        lc.type      = ic::LightType::Directional;
                                        lc.color     = glm::vec3(1.0f, 0.95f, 0.85f);
                                        lc.intensity = 1.0f;
                                }
                                if (ImGui::MenuItem("Spot Light"))
                                {
                                        auto  entity  = scene->CreateEntityWithName("SpotLight_" + std::to_string(n));
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
                std::vector<ic::Entity> entities = scene->GetAllEntities();

                for (auto &e : entities)
                {
                        if (!e.IsValid())
                                continue;

                        std::string label = e.GetName();
                        if (label.empty())
                                label = "(unnamed)";

                        bool isSelected = (state.selected && state.selected == e);

                        ImGui::PushID((int)e.GetUUID());

                        if (state.renameTarget == e)
                        {
                                // Render a leaf node with empty label then put input beside it
                                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
                                                           ImGuiTreeNodeFlags_Selected;
                                ImGui::TreeNodeEx("##node", flags, "");
                                ImGui::TreePop();

                                ImGui::SameLine();

                                if (state.isFocused)
                                {
                                        ImGui::SetKeyboardFocusHere();
                                        state.isFocused = false;
                                }
                                ImGui::SetNextItemWidth(160.0f);
                                if (ImGui::InputText("##rename",
                                                     state.renameBuffer,
                                                     sizeof(state.renameBuffer),
                                                     ImGuiInputTextFlags_AutoSelectAll |
                                                         ImGuiInputTextFlags_EnterReturnsTrue))
                                {
                                        e.SetName(state.renameBuffer);
                                        state.renameTarget = {};
                                }
                                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
                                {
                                        e.SetName(state.renameBuffer);
                                        state.renameTarget = {};
                                }
                        }
                        else
                        {
                                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
                                                           ImGuiTreeNodeFlags_OpenOnArrow |
                                                           (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                                ImGui::TreeNodeEx(label.c_str(), flags);
                                ImGui::TreePop();

                                if (ImGui::IsItemClicked())
                                        state.selected = e;

                                if (ImGui::BeginPopupContextItem("##ctx"))
                                {
                                        if (ImGui::MenuItem("Rename"))
                                        {
                                                state.renameTarget = e;
                                                strncpy(state.renameBuffer,
                                                        e.GetName().c_str(),
                                                        sizeof(state.renameBuffer));
                                                state.renameBuffer[sizeof(state.renameBuffer) - 1] = '\0';
                                                state.isFocused                                    = true;
                                        }
                                        if (ImGui::MenuItem("Duplicate"))
                                        {
                                                ic::Entity dup = scene->CreateEntityWithName(e.GetName() + "_copy");
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
                                                        dup.AddComponent<ic::MeshComponent>().SetMesh(src.modelID);
                                                }
                                                state.selected = dup;
                                        }
                                        ImGui::Separator();
                                        if (ImGui::MenuItem("Delete"))
                                        {
                                                scene->DestroyEntity(e);
                                                if (state.selected && state.selected == e)
                                                        state.selected = {};
                                        }
                                        ImGui::EndPopup();
                                }
                        }

                        ImGui::PopID();
                }

                ImGui::TreePop();
        }

        ImGui::PopID();  // SceneRoot

        ImGui::End();
}

}  // namespace ic::panels
