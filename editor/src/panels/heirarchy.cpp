#include "panels.h"
#include "editor.h"
#include <imgui.h>

// TODO:
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

void EditorSystem::DrawSceneNode()
{
        ImGui::PushID("SceneRoot");

        ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth |
                                       ImGuiTreeNodeFlags_OpenOnArrow;

        bool sceneOpen;

        if (m_State.renamingScene)
        {
                sceneOpen = ImGui::TreeNodeEx("##scenenode", rootFlags | ImGuiTreeNodeFlags_Leaf, "");
                ImGui::SameLine();
                if (DrawRenameField())
                {
                        m_ActiveScene->SetName(m_State.renameBuffer);
                        m_State.renamingScene = false;
                }
        }
        else
        {
                sceneOpen = ImGui::TreeNodeEx("##scenenode", rootFlags, "%s", m_ActiveScene->GetName().c_str());
                DrawSceneContextMenu();
        }

        // The heirarchy of each node if they have mesh components
        if (sceneOpen)
        {
                std::vector<ic::Entity> entities = m_ActiveScene->GetAllEntities();
                for (auto &e : entities)
                {
                        if (!e.IsValid())
                                continue;  // maybe warn here
                        DrawEntityNode(e);
                }

                ImGui::TreePop();
        }
        ImGui::PopID();
}

void EditorSystem::DrawSceneContextMenu()
{
        if (!ImGui::BeginPopupContextItem("##scenectx"))
                return;

        if (ImGui::MenuItem("Rename"))
        {
                m_State.renamingScene = true;
                strncpy(m_State.renameBuffer, m_ActiveScene->GetName().c_str(), sizeof(m_State.renameBuffer));
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
                        auto  entity = m_ActiveScene->CreateEntityWithName("PointLight_" + std::to_string(n));
                        auto &lc     = entity.AddComponent<ic::LightComponent>();
                        lc.type      = ic::LightType::Point;
                        lc.color     = glm::vec3(1.0f);
                        lc.intensity = 1.0f;
                        lc.range     = 10.0f;
                }
                if (ImGui::MenuItem("Directional Light"))
                {
                        auto  entity = m_ActiveScene->CreateEntityWithName("DirectionalLight_" + std::to_string(n));
                        auto &lc     = entity.AddComponent<ic::LightComponent>();
                        lc.type      = ic::LightType::Directional;
                        lc.color     = glm::vec3(1.0f, 0.95f, 0.85f);
                        lc.intensity = 1.0f;
                }
                if (ImGui::MenuItem("Spot Light"))
                {
                        auto  entity  = m_ActiveScene->CreateEntityWithName("SpotLight_" + std::to_string(n));
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

void EditorSystem::DrawEntityNode(ic::Entity e)
{
        ImGui::PushID((int)e.GetUUID());

        if (m_State.renameTarget == e)
        {
                DrawEntityRename(e);
                ImGui::PopID();
                return;
        }

        string label = e.GetName();
        if (label.empty())
                label = "(unnamed)";

        // selected submesh is just a meshprimitive id
        bool entityHighlighted = (m_State.selected && m_State.selected == e && m_State.selectedMesh < 0);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow |
                                   (entityHighlighted ? ImGuiTreeNodeFlags_Selected : 0);

        // TODO: this line may not be needed
        // if (!(e.GetComponent<MeshComponent>().GetMeshes().size() > 0))
        //         flags |= ImGuiTreeNodeFlags_Leaf;  // no expand arrow if there is nothing to expand

        bool open = ImGui::TreeNodeEx(label.c_str(), flags);

        // Selecting the entity clears any submesh sub-selection.
        if (ImGui::IsItemClicked())
        {
                m_State.selected     = e;
                m_State.selectedMesh = -1;
        }

        DrawEntityContextMenu(e);

        if (open)
        {
                DrawMeshNodes(e);
                ImGui::TreePop();
        }

        ImGui::PopID();
}

void EditorSystem::DrawEntityRename(ic::Entity e)
{
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
                                   ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx("##node", flags, "");
        ImGui::TreePop();

        ImGui::SameLine();

        if (DrawRenameField())
        {
                e.SetName(m_State.renameBuffer);
                m_State.renameTarget = {};
        }
}

void EditorSystem::DrawEntityContextMenu(ic::Entity e)
{
        if (!ImGui::BeginPopupContextItem("##ctx"))
                return;

        if (ImGui::MenuItem("Rename"))
        {
                m_State.renameTarget = e;
                strncpy(m_State.renameBuffer, e.GetName().c_str(), sizeof(m_State.renameBuffer));
                m_State.renameBuffer[sizeof(m_State.renameBuffer) - 1] = '\0';
                m_State.isFocused                                      = true;
        }
        if (ImGui::MenuItem("Duplicate"))
        {
                ic::Entity dup = m_ActiveScene->CreateEntityWithName(e.GetName() + "_copy");
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
                        dup.AddComponent<ic::MeshComponent>().SetModel(src.modelID);
                }
                m_State.selected     = dup;
                m_State.selectedMesh = -1;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
        {
                m_ActiveScene->DestroyEntity(e);
                if (m_State.selected && m_State.selected == e)
                {
                        m_State.selected     = {};
                        m_State.selectedMesh = -1;
                }
        }
        ImGui::EndPopup();
}

void EditorSystem::DrawMeshNodes(ic::Entity e)
{
        std::vector<ic::Mesh> meshes;
        if (e.HasComponent<MeshComponent>())
        {
                meshes = e.GetComponent<MeshComponent>().GetMeshes();
        }

        for (size_t i = 0; i < meshes.size(); ++i)
        {
                ImGui::PushID((int)i);

                bool subSelected = (m_State.selected && m_State.selected == e && m_State.selectedMesh == (int)i);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
                                           (subSelected ? ImGuiTreeNodeFlags_Selected : 0);

                ImGui::TreeNodeEx(meshes[i].name.c_str(), flags);

                if (ImGui::IsItemClicked())
                {
                        m_State.selected     = e;
                        m_State.selectedMesh = (int)i;

                        Mesh &mesh = meshes[i];

                        DrawSubmeshNodes(e, mesh, i);

                        /** TODO: In the component section we need a material selector */
                        // DrawMaterialPanel(meshId);
                        /**
                         * {
                         * 	mesh = getmesh();
                         * 	if (choosematerial) openassetbrowser
                         * 	choose an asset / material (will have all materials that are loaded yet with each
                         * catergories as well) load it and hot reload it into the scene make a shader asset as well.
                         * }
                         */
                }

                ImGui::TreePop();
                ImGui::PopID();
        }
}

void EditorSystem::DrawSubmeshNodes(ic::Entity e, Mesh &mesh, int selectedMesh)
{
        std::vector<MeshPrimitive> &submeshes = mesh.primitives;
        for (size_t j = 0; j < submeshes.size(); ++j)
        {
                // TODO: learn if this breaks anything
                ImGui::PushID((int)(selectedMesh + j));

                bool subSelected = (m_State.selected && m_State.selected == e && m_State.selectedMesh == selectedMesh &&
                                    m_State.selectedSubmesh == j);
                ;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth |
                                           (subSelected ? ImGuiTreeNodeFlags_Selected : 0);

                ImGui::TreeNodeEx(submeshes[j].name.c_str(), flags);

                if (ImGui::IsItemClicked())
                {
                        m_State.selected        = e;
                        m_State.selectedMesh    = selectedMesh;
                        m_State.selectedSubmesh = j;

                        ImGui::SetTooltip("This is a submeshh");
                }

                ImGui::TreePop();
                ImGui::PopID();
        }
}

bool EditorSystem::DrawRenameField()
{
        if (m_State.isFocused)
        {
                ImGui::SetKeyboardFocusHere();
                m_State.isFocused = false;
        }

        ImGui::SetNextItemWidth(160.0f);
        bool committed = ImGui::InputText("##rename",
                                          m_State.renameBuffer,
                                          sizeof(m_State.renameBuffer),
                                          ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
                committed = true;

        return committed;
}

void EditorSystem::DrawCameraPanel()
{
        float cameraStartY = ImGui::GetCursorPosY();
        ImGui::Separator();
        ic::panels::camera_panel_draw(m_EditorCamera);
        float cameraEndY = ImGui::GetCursorPosY();

        m_State.lastCameraPanelHeight = cameraEndY - cameraStartY;
}

void EditorSystem::DrawSceneHeirarchy()
{
        ImGui::Begin("Hierarchy");

        float availHeight     = ImGui::GetContentRegionAvail().y;
        float hierarchyHeight = availHeight - m_State.lastCameraPanelHeight;
        if (hierarchyHeight < 0.0f)
                hierarchyHeight = 0.0f;

        ImGui::BeginChild("SceneHierarchyRegion", ImVec2(0, hierarchyHeight), false);
        if (m_ActiveScene)
                DrawSceneNode();
        ImGui::EndChild();

        DrawCameraPanel();

        ImGui::End();
}

}  // namespace ic
